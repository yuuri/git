/*
 * GIT - The information manager from hell
 *
 * Copyright (C) Linus Torvalds, 2005
 */
#include "git-compat-util.h"
#include "cache.h"

static void replace_control_chars(char* str, size_t size, char replacement)
{
	size_t i;

	for (i = 0; i < size; i++) {
    		if (iscntrl(str[i]) && str[i] != '\t' && str[i] != '\n')
			str[i] = replacement;
	}
}

/*
 * Atomically report (prefix + vsnprintf(err, params) + '\n') to stderr.
 * Always returns desired buffer size.
 * Doesn't write to stderr if content didn't fit.
 *
 * This function composes everything into a single buffer before
 * sending to stderr. This is to defeat various non-atomic issues:
 * 1) If stderr is fully buffered:
 *    the ordering of stdout and stderr messages could be wrong,
 *    because stderr output waits for the buffer to become full.
 * 2) If stderr has any type of buffering:
 *    buffer has fixed size, which could lead to interleaved buffer
 *    blocks when two threads/processes write at the same time.
 * 3) If stderr is not buffered:
 *    There are two problems, one with atomic fprintf() and another
 *    for non-atomic fprintf(), and both occur depending on platform
 *    (see details below). If atomic, this function still writes 3
 *    parts, which could get interleaved with multiple threads. If
 *    not atomic, then fprintf() will basically write char-by-char,
 *    which leads to unreadable char-interleaved writes if two
 *    processes write to stderr at the same time (threads are OK
 *    because fprintf() usually locks file in current process). This
 *    for example happens in t5516 where 'git-upload-pack' detects
 *    an error, reports it to parent 'git fetch' and both die() at the
 *    same time.
 *
 *    Behaviors, at the moment of writing:
 *    a) libc - fprintf()-interleaved
 *       fprintf() enables temporary stream buffering.
 *       See: buffered_vfprintf()
 *    b) VC++ - char-interleaved
 *       fprintf() enables temporary stream buffering, but only if
 *       stream was not set to no buffering. This has no effect,
 *       because stderr is not buffered by default, and git takes
 *       an extra step to ensure that in swap_osfhnd().
 *       See: _iob[_IOB_ENTRIES],
 *            __acrt_stdio_temporary_buffering_guard,
 *            has_any_buffer()
 *    c) MinGW - char-interleaved (console), full buffering (file)
 *       fprintf() obeys stderr buffering. But it uses old MSVCRT.DLL,
 *       which eventually calls _flsbuf(), which enables buffering unless
 *       isatty(stderr) or buffering is disabled. Buffering is not disabled
 *       by default for stderr. Therefore, buffering is enabled for
 *       file-redirected stderr.
 *       See: __mingw_vfprintf(),
 *            __pformat_wcputs(),
 *            _fputc_nolock(),
 *            _flsbuf(),
 *            _iob[_IOB_ENTRIES]
 * 4) If stderr is line buffered: MinGW/VC++ will enable full
 *    buffering instead. See MSDN setvbuf().
 */
static size_t vreportf_buf(char *buf, size_t buf_size, const char *prefix, const char *err, va_list params)
{
	int printf_ret = 0;
	size_t prefix_size = 0;
	size_t total_size = 0;

	/*
	 * NOTE: Can't use strbuf functions here, because it can be called when
	 * malloc() is no longer possible, and strbuf will recurse die().
	 */

	/* Prefix */
	prefix_size = strlen(prefix);
	if (total_size + prefix_size <= buf_size)
		memcpy(buf + total_size, prefix, prefix_size);
	
	total_size += prefix_size;

	/* Formatted message */
	if (total_size <= buf_size)
		printf_ret = vsnprintf(buf + total_size, buf_size - total_size, err, params);
	else
		printf_ret = vsnprintf(NULL, 0, err, params);

	if (printf_ret < 0)
		BUG("your vsnprintf is broken (returned %d)", printf_ret);

	/*
	 * vsnprintf() returns _desired_ size (without terminating null).
	 * If vsnprintf() was truncated that will be seen when appending '\n'.
	 */
	total_size += printf_ret;

	/* Trailing \n */
	if (total_size + 1 <= buf_size)
		buf[total_size] = '\n';

	total_size += 1;

	/* Send the buffer, if content fits */
	if (total_size <= buf_size) {
	    replace_control_chars(buf, total_size, '?');
	    fwrite(buf, total_size, 1, stderr);
	}

	return total_size;
}

void vreportf(const char *prefix, const char *err, va_list params)
{
	size_t res = 0;
	char *buf = NULL;
	size_t buf_size = 0;

	/*
	 * NOTE: This can be called from failed xmalloc(). Any malloc() can
	 * fail now. Let's try to report with a fixed size stack based buffer.
	 * Also, most messages should fit, and this path is faster.
	 */
	char msg[4096];
	res = vreportf_buf(msg, sizeof(msg), prefix, err, params);
	if (res <= sizeof(msg)) {
		/* Success */
		return;
	}

	/*
	 * Try to allocate a suitable sized malloc(), if possible.
	 * NOTE: Do not use xmalloc(), because on failure it will call
	 * die() or warning() and lead to recursion.
	 */
	buf_size = res;
	buf = malloc(buf_size);
	if (buf) {
		res = vreportf_buf(buf, buf_size, prefix, err, params);
		FREE_AND_NULL(buf);

		if (res <= buf_size) {
			/* Success */
			return;
		}
	}

	/* 
	 * When everything fails, report in parts.
	 * This can have all problems prevented by vreportf_buf().
	 */
	fprintf(stderr, "vreportf: not enough memory (tried to allocate %lu bytes)\n", (unsigned long)buf_size);
	fputs(prefix, stderr);
	vfprintf(stderr, err, params);
	fputc('\n', stderr);
}

static NORETURN void usage_builtin(const char *err, va_list params)
{
	vreportf("usage: ", err, params);

	/*
	 * When we detect a usage error *before* the command dispatch in
	 * cmd_main(), we don't know what verb to report.  Force it to this
	 * to facilitate post-processing.
	 */
	trace2_cmd_name("_usage_");

	/*
	 * Currently, the (err, params) are usually just the static usage
	 * string which isn't very useful here.  Usually, the call site
	 * manually calls fprintf(stderr,...) with the actual detailed
	 * syntax error before calling usage().
	 *
	 * TODO It would be nice to update the call sites to pass both
	 * the static usage string and the detailed error message.
	 */

	exit(129);
}

static NORETURN void die_builtin(const char *err, va_list params)
{
	/*
	 * We call this trace2 function first and expect it to va_copy 'params'
	 * before using it (because an 'ap' can only be walked once).
	 */
	trace2_cmd_error_va(err, params);

	vreportf("fatal: ", err, params);

	exit(128);
}

static void error_builtin(const char *err, va_list params)
{
	/*
	 * We call this trace2 function first and expect it to va_copy 'params'
	 * before using it (because an 'ap' can only be walked once).
	 */
	trace2_cmd_error_va(err, params);

	vreportf("error: ", err, params);
}

static void warn_builtin(const char *warn, va_list params)
{
	vreportf("warning: ", warn, params);
}

static int die_is_recursing_builtin(void)
{
	static int dying;
	/*
	 * Just an arbitrary number X where "a < x < b" where "a" is
	 * "maximum number of pthreads we'll ever plausibly spawn" and
	 * "b" is "something less than Inf", since the point is to
	 * prevent infinite recursion.
	 */
	static const int recursion_limit = 1024;

	dying++;
	if (dying > recursion_limit) {
		return 1;
	} else if (dying == 2) {
		warning("die() called many times. Recursion error or racy threaded death!");
		return 0;
	} else {
		return 0;
	}
}

/* If we are in a dlopen()ed .so write to a global variable would segfault
 * (ugh), so keep things static. */
static NORETURN_PTR void (*usage_routine)(const char *err, va_list params) = usage_builtin;
static NORETURN_PTR void (*die_routine)(const char *err, va_list params) = die_builtin;
static void (*error_routine)(const char *err, va_list params) = error_builtin;
static void (*warn_routine)(const char *err, va_list params) = warn_builtin;
static int (*die_is_recursing)(void) = die_is_recursing_builtin;

void set_die_routine(NORETURN_PTR void (*routine)(const char *err, va_list params))
{
	die_routine = routine;
}

void set_error_routine(void (*routine)(const char *err, va_list params))
{
	error_routine = routine;
}

void (*get_error_routine(void))(const char *err, va_list params)
{
	return error_routine;
}

void set_warn_routine(void (*routine)(const char *warn, va_list params))
{
	warn_routine = routine;
}

void (*get_warn_routine(void))(const char *warn, va_list params)
{
	return warn_routine;
}

void set_die_is_recursing_routine(int (*routine)(void))
{
	die_is_recursing = routine;
}

void NORETURN usagef(const char *err, ...)
{
	va_list params;

	va_start(params, err);
	usage_routine(err, params);
	va_end(params);
}

void NORETURN usage(const char *err)
{
	usagef("%s", err);
}

void NORETURN die(const char *err, ...)
{
	va_list params;

	if (die_is_recursing()) {
		fputs("fatal: recursion detected in die handler\n", stderr);
		exit(128);
	}

	va_start(params, err);
	die_routine(err, params);
	va_end(params);
}

static const char *fmt_with_err(char *buf, int n, const char *fmt)
{
	char str_error[256], *err;
	int i, j;

	err = strerror(errno);
	for (i = j = 0; err[i] && j < sizeof(str_error) - 1; ) {
		if ((str_error[j++] = err[i++]) != '%')
			continue;
		if (j < sizeof(str_error) - 1) {
			str_error[j++] = '%';
		} else {
			/* No room to double the '%', so we overwrite it with
			 * '\0' below */
			j--;
			break;
		}
	}
	str_error[j] = 0;
	/* Truncation is acceptable here */
	snprintf(buf, n, "%s: %s", fmt, str_error);
	return buf;
}

void NORETURN die_errno(const char *fmt, ...)
{
	char buf[1024];
	va_list params;

	if (die_is_recursing()) {
		fputs("fatal: recursion detected in die_errno handler\n",
			stderr);
		exit(128);
	}

	va_start(params, fmt);
	die_routine(fmt_with_err(buf, sizeof(buf), fmt), params);
	va_end(params);
}

#undef error_errno
int error_errno(const char *fmt, ...)
{
	char buf[1024];
	va_list params;

	va_start(params, fmt);
	error_routine(fmt_with_err(buf, sizeof(buf), fmt), params);
	va_end(params);
	return -1;
}

#undef error
int error(const char *err, ...)
{
	va_list params;

	va_start(params, err);
	error_routine(err, params);
	va_end(params);
	return -1;
}

void warning_errno(const char *warn, ...)
{
	char buf[1024];
	va_list params;

	va_start(params, warn);
	warn_routine(fmt_with_err(buf, sizeof(buf), warn), params);
	va_end(params);
}

void warning(const char *warn, ...)
{
	va_list params;

	va_start(params, warn);
	warn_routine(warn, params);
	va_end(params);
}

/* Only set this, ever, from t/helper/, when verifying that bugs are caught. */
int BUG_exit_code;

static NORETURN void BUG_vfl(const char *file, int line, const char *fmt, va_list params)
{
	char prefix[256];

	/* truncation via snprintf is OK here */
	if (file)
		snprintf(prefix, sizeof(prefix), "BUG: %s:%d: ", file, line);
	else
		snprintf(prefix, sizeof(prefix), "BUG: ");

	vreportf(prefix, fmt, params);
	if (BUG_exit_code)
		exit(BUG_exit_code);
	abort();
}

#ifdef HAVE_VARIADIC_MACROS
NORETURN void BUG_fl(const char *file, int line, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	BUG_vfl(file, line, fmt, ap);
	va_end(ap);
}
#else
NORETURN void BUG(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	BUG_vfl(NULL, 0, fmt, ap);
	va_end(ap);
}
#endif

#ifdef SUPPRESS_ANNOTATED_LEAKS
void unleak_memory(const void *ptr, size_t len)
{
	static struct suppressed_leak_root {
		struct suppressed_leak_root *next;
		char data[FLEX_ARRAY];
	} *suppressed_leaks;
	struct suppressed_leak_root *root;

	FLEX_ALLOC_MEM(root, data, ptr, len);
	root->next = suppressed_leaks;
	suppressed_leaks = root;
}
#endif
