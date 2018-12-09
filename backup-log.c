#include "cache.h"
#include "backup-log.h"
#include "lockfile.h"
#include "strbuf.h"

void bkl_append(struct strbuf *output, const char *path,
		const struct object_id *from,
		const struct object_id *to)
{
	if (oideq(from, to))
		return;

	strbuf_addf(output, "%s %s %s\t%s\n", oid_to_hex(from),
		    oid_to_hex(to), git_committer_info(0), path);
}

static int bkl_write_unlocked(const char *path, struct strbuf *new_log)
{
	int fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0666);
	if (fd == -1)
		return error_errno(_("unable to open %s"), path);
	if (write_in_full(fd, new_log->buf, new_log->len) < 0) {
		close(fd);
		return error_errno(_("unable to update %s"), path);
	}
	close(fd);
	return 0;
}

int bkl_write(const char *path, struct strbuf *new_log)
{
	struct lock_file lk;
	int ret;

	ret = hold_lock_file_for_update(&lk, path, LOCK_REPORT_ON_ERROR);
	if (ret == -1)
		return -1;
	ret = bkl_write_unlocked(path, new_log);
	/*
	 * We do not write the the .lock file and append to the real one
	 * instead to reduce update cost. So we can't commit even in
	 * successful case.
	 */
	rollback_lock_file(&lk);
	return ret;
}
