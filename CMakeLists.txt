#
#	Copyright (c) 2020 Sibi Siddharthan
#

cmake_minimum_required(VERSION 3.15)

#Parse GIT-VERSION-GEN to get the version
file(STRINGS ${CMAKE_SOURCE_DIR}/GIT-VERSION-GEN git_version REGEX "DEF_VER=v(.*)")
string(REPLACE "DEF_VER=v" "" git_version ${git_version})
string(REPLACE ".GIT" ".0" git_version ${git_version})#for building from a snapshot

project(git
	VERSION ${git_version}
	LANGUAGES C)


#TODO gitk git-gui gitweb
#TODO Enable NLS on windows natively
#TODO Add pcre support


include(CheckTypeSize)
include(CheckCSourceRuns)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(CTest)

find_package(ZLIB REQUIRED)
find_package(CURL)
find_package(EXPAT)
find_package(Iconv)

#Don't use libintl on Windows Visual Studio and Clang builds
if(NOT (WIN32 AND (CMAKE_C_COMPILER_ID STREQUAL "MSVC" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")))
	find_package(Intl)
endif()

if(NOT Intl_FOUND)
	add_compile_definitions(NO_GETTEXT)
	if(NOT Iconv_FOUND)
		add_compile_definitions(NO_ICONV)
	endif()
endif()

include_directories(SYSTEM ${ZLIB_INCLUDE_DIRS})
if(CURL_FOUND)
	include_directories(SYSTEM ${CURL_INCLUDE_DIRS})
endif()
if(EXPAT_FOUND)
	include_directories(SYSTEM ${EXPAT_INCLUDE_DIRS})
endif()
if(Iconv_FOUND)
	include_directories(SYSTEM ${Iconv_INCLUDE_DIRS})
endif()
if(Intl_FOUND)
	include_directories(SYSTEM ${Intl_INCLUDE_DIRS})
endif()

find_program(SH_EXE sh)
if(NOT SH_EXE)
	message(FATAL_ERROR "sh: shell interpreter was not found in your path, please install one."
			"On Windows, you can get it as part of 'Git for Windows' install at https://gitforwindows.org/")
endif()

if(WIN32 AND NOT MSVC)#not required for visual studio builds
	find_program(WINDRES_EXE windres)
	if(NOT WINDRES_EXE)
		message(FATAL_ERROR "Install windres on Windows for resource files")
	endif()
endif()

find_program(MSGFMT_EXE msgfmt)
if(NOT MSGFMT_EXE)
	message(WARNING "Text Translations won't be build")
endif()

#Force all visual studio outputs to CMAKE_BINARY_DIR
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
	add_compile_options(/MP)
endif()

#default behaviour
include_directories(${CMAKE_SOURCE_DIR})
add_compile_definitions(GIT_HOST_CPU="${CMAKE_SYSTEM_PROCESSOR}")
add_compile_definitions(SHA256_BLK INTERNAL_QSORT RUNTIME_PREFIX)
add_compile_definitions(NO_OPENSSL SHA1_DC SHA1DC_NO_STANDARD_INCLUDES
			SHA1DC_INIT_SAFE_HASH_DEFAULT=0
			SHA1DC_CUSTOM_INCLUDE_SHA1_C="cache.h"
			SHA1DC_CUSTOM_INCLUDE_UBC_CHECK_C="git-compat-util.h" )
list(APPEND compat_SOURCES sha1dc_git.c sha1dc/sha1.c sha1dc/ubc_check.c block-sha1/sha1.c sha256/block/sha256.c compat/qsort_s.c)


add_compile_definitions(PAGER_ENV="LESS=FRX LV=-c"
			ETC_GITATTRIBUTES="etc/gitattributes"
			ETC_GITCONFIG="etc/gitconfig"
			GIT_EXEC_PATH="libexec/git-core"
			GIT_LOCALE_PATH="share/locale"
			GIT_MAN_PATH="share/man"
			GIT_INFO_PATH="share/info"
			GIT_HTML_PATH="share/doc/git-doc"
			DEFAULT_HELP_FORMAT="html"
			DEFAULT_GIT_TEMPLATE_DIR="share/git-core/templates"
			GIT_VERSION="${PROJECT_VERSION}.GIT"
			GIT_USER_AGENT="git/${PROJECT_VERSION}.GIT"
			BINDIR="bin"
			GIT_BUILT_FROM_COMMIT="")

if(WIN32)
	set(FALLBACK_RUNTIME_PREFIX /mingw64)
	add_compile_definitions(FALLBACK_RUNTIME_PREFIX="${FALLBACK_RUNTIME_PREFIX}")
else()
	set(FALLBACK_RUNTIME_PREFIX /home/$ENV{USER})
	add_compile_definitions(FALLBACK_RUNTIME_PREFIX="${FALLBACK_RUNTIME_PREFIX}")
endif()


#Platform Specific
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	if(CMAKE_C_COMPILER_ID STREQUAL "MSVC" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
		include_directories(compat/vcbuild/include)
		add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
	endif()
	include_directories(compat/win32)
	add_compile_definitions(HAVE_ALLOCA_H NO_POSIX_GOODIES NATIVE_CRLF NO_UNIX_SOCKETS WIN32
				_CONSOLE DETECT_MSYS_TTY STRIP_EXTENSION=".exe"  NO_SYMLINK_HEAD UNRELIABLE_FSTAT
				NOGDI OBJECT_CREATION_MODE=1 __USE_MINGW_ANSI_STDIO=0
				USE_NED_ALLOCATOR OVERRIDE_STRDUP MMAP_PREVENTS_DELETE USE_WIN32_MMAP
				UNICODE _UNICODE HAVE_WPGMPTR ENSURE_MSYSTEM_IS_SET)
	list(APPEND compat_SOURCES compat/mingw.c compat/winansi.c compat/win32/path-utils.c
		compat/win32/pthread.c compat/win32mmap.c compat/win32/syslog.c
		compat/win32/trace2_win32_process_info.c compat/win32/dirent.c
		compat/nedmalloc/nedmalloc.c compat/strdup.c)
	set(NO_UNIX_SOCKETS 1)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	add_compile_definitions(PROCFS_EXECUTABLE_PATH="/proc/self/exe" HAVE_DEV_TTY )
	list(APPEND compat_SOURCES unix-socket.c)
endif()

if(WIN32)
	set(EXE_EXTENSION .exe)
else()
	set(EXE_EXTENSION)
endif()

#header checks
check_include_file(libgen.h HAVE_LIBGEN_H)
if(NOT HAVE_LIBGEN_H)
	add_compile_definitions(NO_LIBGEN_H)
	list(APPEND compat_SOURCES compat/basename.c)
endif()

check_include_file(sys/sysinfo.h HAVE_SYSINFO)
if(HAVE_SYSINFO)
	add_compile_definitions(HAVE_SYSINFO)
endif()

check_c_source_compiles("
#include <alloca.h>
int
main ()
{
char *p = (char *) alloca (2 * sizeof (int));
	if (p) return 0;
	return 0;
}"
HAVE_ALLOCA_H)
if(HAVE_ALLOCA_H)
	add_compile_definitions(HAVE_ALLOCA_H)
endif()

check_include_file(strings.h HAVE_STRINGS_H)
if(HAVE_STRINGS_H)
	add_compile_definitions(HAVE_STRINGS_H)
endif()

check_include_file(sys/select.h HAVE_SYS_SELECT_H)
if(NOT HAVE_SYS_SELECT_H)
	add_compile_definitions(NO_SYS_SELECT_H)
endif()

check_include_file(sys/poll.h HAVE_SYS_POLL_H)
if(NOT HAVE_SYS_POLL_H)
	add_compile_definitions(NO_SYS_POLL_H)
endif()

check_include_file(poll.h HAVE_POLL_H)
if(NOT HAVE_POLL_H)
	add_compile_definitions(NO_POLL_H)
endif()

check_include_file(inttypes.h HAVE_INTTYPES_H)
if(NOT HAVE_INTTYPES_H)
	add_compile_definitions(NO_INTTYPES_H)
endif()

check_include_file(paths.h HAVE_PATHS_H)
if(HAVE_PATHS_H)
	add_compile_definitions(HAVE_PATHS_H)
endif()

#function checks
set(function_checks
	strcasestr memmem strlcpy strtoimax strtoumax strtoull
	setenv  mkdtemp poll pread  memmem)

#unsetenv,hstrerror are incompatible with windows build
if(NOT WIN32)
	list(APPEND function_checks unsetenv hstrerror)
endif()

foreach(f ${function_checks})
	string(TOUPPER ${f} uf)
	check_function_exists(${f} HAVE_${uf})
	if(NOT HAVE_${uf})
		add_compile_definitions(NO_${uf})
	endif()
endforeach()

if(NOT HAVE_POLL_H OR NOT HAVE_SYS_POLL_H OR NOT HAVE_POLL)
	include_directories(compat/poll)
	add_compile_definitions(NO_POLL)
	list(APPEND compat_SOURCES compat/poll/poll.c)
endif()

if(NOT HAVE_STRCASESTR)
	list(APPEND compat_SOURCES compat/strcasestr.c)
endif()

if(NOT HAVE_STRLCPY)
	list(APPEND compat_SOURCES compat/strlcpy.c)
endif()

if(NOT HAVE_STRTOUMAX)
	list(APPEND compat_SOURCES compat/strtoumax.c compat/strtoimax.c)
endif()

if(NOT HAVE_SETENV)
	list(APPEND compat_SOURCES compat/setenv.c)
endif()

if(NOT HAVE_MKDTEMP)
	list(APPEND compat_SOURCES compat/mkdtemp.c)
endif()

if(NOT HAVE_PREAD)
	list(APPEND compat_SOURCES compat/pread.c)
endif()

if(NOT HAVE_MEMMEM)
	list(APPEND compat_SOURCES compat/memmem.c)
endif()

if(NOT WIN32)
	if(NOT HAVE_UNSETENV)
		list(APPEND compat_SOURCES compat/unsetenv.c)
	endif()

	if(NOT HAVE_HSTRERROR)
		list(APPEND compat_SOURCES compat/hstrerror.c)
	endif()
endif()

check_function_exists(getdelim HAVE_GETDELIM)
if(HAVE_GETDELIM)
	add_compile_definitions(HAVE_GETDELIM)
endif()

check_function_exists(clock_gettime HAVE_CLOCK_GETTIME)
check_symbol_exists(CLOCK_MONOTONIC "time.h" HAVE_CLOCK_MONOTONIC)
if(HAVE_CLOCK_GETTIME)
	add_compile_definitions(HAVE_CLOCK_GETTIME)
endif()
if(HAVE_CLOCK_MONOTONIC)
	add_compile_definitions(HAVE_CLOCK_MONOTONIC)
endif()

#check for st_blocks in struct stat
check_struct_has_member("struct stat" st_blocks "sys/stat.h" STRUCT_STAT_HAS_ST_BLOCKS)
if(NOT STRUCT_STAT_HAS_ST_BLOCKS)
	add_compile_definitions(NO_ST_BLOCKS_IN_STRUCT_STAT)
endif()

#compile checks
check_c_source_runs("
#include<stdio.h>
#include<stdarg.h>
#include<string.h>
#include<stdlib.h>
int test_vsnprintf(char *str, size_t maxsize, const char *format, ...)
{
	int ret;
	va_list ap;

	va_start(ap, format);
	ret = vsnprintf(str, maxsize, format, ap);
	va_end(ap);
	return ret;
}

int
main ()
{
	char buf[6];
	if (test_vsnprintf(buf, 3, \"%s\", \"12345\") != 5
		|| strcmp(buf, \"12\")) return 1;
	if (snprintf(buf, 3, \"%s\", \"12345\") != 5
		|| strcmp(buf, \"12\")) return 1;

	return 0;
}"
SNPRINTF_OK)
if(NOT SNPRINTF_OK)
	add_compile_definitions(SNPRINTF_RETURNS_BOGUS)
	list(APPEND compat_SOURCES compat/snprintf.c)
endif()

check_c_source_runs("
#include<stdio.h>
int
main ()
{
	FILE *f = fopen(\".\", \"r\");
	return f != NULL;

	return 0;
}"
FREAD_READS_DIRECTORIES_NO)
if(NOT FREAD_READS_DIRECTORIES_NO)
	add_compile_definitions(FREAD_READS_DIRECTORIES)
	list(APPEND compat_SOURCES compat/fopen.c)
endif()

check_c_source_compiles("
#include <regex.h>
#ifndef REG_STARTEND
#error oops we dont have it
#endif
int main(){return 0;}"
HAVE_REGEX)
if(NOT HAVE_REGEX)
	include_directories(compat/regex )
	list(APPEND compat_SOURCES compat/regex/regex.c )
	add_compile_definitions(NO_REGEX NO_MBSUPPORT GAWK)
endif()

check_c_source_compiles("
#include <stddef.h>
#include <sys/types.h>
#include <sys/sysctl.h>

int
main ()
{
	int val, mib[2];
	size_t len;

	mib[0] = CTL_HW;
	mib[1] = 1;
	len = sizeof(val);
	return sysctl(mib, 2, &val, &len, NULL, 0) ? 1 : 0;

	return 0;
}"
HAVE_BSD_SYSCTL)
if(HAVE_BSD_SYSCTL)
	add_compile_definitions(HAVE_BSD_SYSCTL)
endif()

set(CMAKE_REQUIRED_LIBRARIES ${Iconv_LIBRARIES})
set(CMAKE_REQUIRED_INCLUDES ${Iconv_INCLUDE_DIRS})

check_c_source_compiles("
#include <iconv.h>

extern size_t iconv(iconv_t cd,
		char **inbuf, size_t *inbytesleft,
		char **outbuf, size_t *outbytesleft);

int main(){return 0;}"
HAVE_NEW_ICONV)
if(HAVE_NEW_ICONV)
	set(HAVE_OLD_ICONV 0)
else()
	set(HAVE_OLD_ICONV 1)
endif()

check_c_source_runs("
#include <iconv.h>
#if ${HAVE_OLD_ICONV}
typedef const char *iconv_ibp;
#else
typedef char *iconv_ibp;
#endif

int main()
{
	int v;
	iconv_t conv;
	char in[] = \"a\"; iconv_ibp pin = in;
	char out[20] = \"\"; char *pout = out;
	size_t isz = sizeof in;
	size_t osz = sizeof out;

	conv = iconv_open(\"UTF-16\", \"UTF-8\");
	iconv(conv, &pin, &isz, &pout, &osz);
	iconv_close(conv);
	v = (unsigned char)(out[0]) + (unsigned char)(out[1]);
	return v != 0xfe + 0xff;
}"
ICONV_DOESNOT_OMIT_BOM)
if(NOT ICONV_DOESNOT_OMIT_BOM)
	add_compile_definitions(ICONV_OMITS_BOM)
endif()

unset(CMAKE_REQUIRED_LIBRARIES)
unset(CMAKE_REQUIRED_INCLUDES)


#programs
set(PROGRAMS_BUILT
	git git-bugreport git-credential-store git-daemon git-fast-import git-http-backend git-sh-i18n--envsubst
	git-shell git-remote-testsvn)

if(NO_UNIX_SOCKETS)
	list(APPEND excluded_progs git-credential-cache git-credential-cache--daemon)
else()
	list(APPEND PROGRAMS_BUILT git-credential-cache git-credential-cache--daemon)
endif()

if(NOT CURL_FOUND)
	list(APPEND excluded_progs git-http-fetch git-http-push)
	add_compile_definitions(NO_CURL)
	message(WARNING "git-http-push and git-http-fetch will not be built")
else()
	list(APPEND PROGRAMS_BUILT git-http-fetch git-http-push git-imap-send git-remote-http)
	if(CURL_VERSION_STRING VERSION_GREATER_EQUAL 7.34.0)
		add_compile_definitions(USE_CURL_FOR_IMAP_SEND)
	endif()
endif()

if(NOT EXPAT_FOUND)
	list(APPEND excluded_progs git-http-push)
	add_compile_definitions(NO_EXPAT)
else()
	list(APPEND PROGRAMS_BUILT git-http-push)
	if(EXPAT_VERSION_STRING VERSION_LESS_EQUAL 1.2)
		add_compile_definitions(EXPAT_NEEDS_XMLPARSE_H)
	endif()
endif()

list(REMOVE_DUPLICATES excluded_progs)
list(REMOVE_DUPLICATES PROGRAMS_BUILT)


foreach(p ${excluded_progs})
	list(APPEND EXCLUSION_PROGS --exclude-program ${p} )
endforeach()

#for comparing null values
list(APPEND EXCLUSION_PROGS empty)
set(EXCLUSION_PROGS_CACHE ${EXCLUSION_PROGS} CACHE STRING "Programs not built" FORCE)

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/command-list.h OR NOT EXCLUSION_PROGS_CACHE STREQUAL EXCLUSION_PROGS)
	list(REMOVE_ITEM EXCLUSION_PROGS empty)
	message("Generating command-list.h")
	execute_process(COMMAND ${SH_EXE} ${CMAKE_SOURCE_DIR}/generate-cmdlist.sh ${EXCLUSION_PROGS} command-list.txt
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			OUTPUT_FILE ${CMAKE_SOURCE_DIR}/command-list.h)
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/config-list.h)
	message("Generating config-list.h")
	execute_process(COMMAND ${SH_EXE} ${CMAKE_SOURCE_DIR}/generate-configlist.sh
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			OUTPUT_FILE ${CMAKE_SOURCE_DIR}/config-list.h)
endif()


#build
set(libgit_SOURCES
	abspath.c add-interactive.c add-patch.c advice.c alias.c
	alloc.c apply.c archive.c archive-tar.c archive-zip.c argv-array.c
	attr.c base85.c bisect.c blame.c blob.c bloom.c branch.c bulk-checkin.c
	bundle.c cache-tree.c chdir-notify.c checkout.c color.c column.c
	combine-diff.c commit.c commit-graph.c commit-reach.c compat/obstack.c
	compat/terminal.c config.c connect.c connected.c convert.c copy.c credential.c
	csum-file.c ctype.c date.c decorate.c delta-islands.c diffcore-break.c
	diffcore-delta.c diffcore-order.c diffcore-pickaxe.c diffcore-rename.c
	diff-delta.c diff-lib.c diff-no-index.c diff.c dir.c dir-iterator.c editor.c
	entry.c environment.c ewah/bitmap.c ewah/ewah_bitmap.c ewah/ewah_io.c
	ewah/ewah_rlw.c exec-cmd.c fetch-negotiator.c fetch-pack.c fmt-merge-msg.c fsck.c fsmonitor.c
	gettext.c gpg-interface.c graph.c grep.c hashmap.c linear-assignment.c help.c hex.c
	ident.c interdiff.c json-writer.c kwset.c levenshtein.c line-log.c line-range.c list-objects.c
	list-objects-filter.c list-objects-filter-options.c ll-merge.c lockfile.c
	log-tree.c ls-refs.c mailinfo.c mailmap.c match-trees.c mem-pool.c merge.c merge-blobs.c
	merge-recursive.c mergesort.c midx.c name-hash.c negotiator/default.c
	negotiator/skipping.c notes.c notes-cache.c notes-merge.c notes-utils.c object.c oidmap.c
	oidset.c oid-array.c packfile.c pack-bitmap.c pack-bitmap-write.c pack-check.c pack-objects.c
	pack-revindex.c pack-write.c pager.c parse-options.c parse-options-cb.c patch-delta.c
	patch-ids.c path.c pathspec.c pkt-line.c preload-index.c pretty.c prio-queue.c progress.c
	promisor-remote.c prompt.c protocol.c prune-packed.c quote.c range-diff.c reachable.c read-cache.c rebase.c
	rebase-interactive.c reflog-walk.c refs.c refs/files-backend.c refs/iterator.c
	refs/packed-backend.c refs/ref-cache.c refspec.c ref-filter.c remote.c replace-object.c
	repo-settings.c repository.c rerere.c reset.c resolve-undo.c revision.c run-command.c
	send-pack.c sequencer.c serve.c server-info.c setup.c sha1-lookup.c
	sha1-file.c sha1-name.c shallow.c sideband.c sigchain.c split-index.c
	stable-qsort.c strbuf.c streaming.c string-list.c submodule.c submodule-config.c
	sub-process.c symlinks.c tag.c tempfile.c thread-utils.c tmp-objdir.c
	trace.c trace2.c trace2/tr2_cfg.c trace2/tr2_cmd_name.c trace2/tr2_dst.c
	trace2/tr2_sid.c trace2/tr2_sysenv.c trace2/tr2_tbuf.c trace2/tr2_tgt_event.c
	trace2/tr2_tgt_normal.c trace2/tr2_tgt_perf.c trace2/tr2_tls.c trailer.c transport.c
	transport-helper.c tree-diff.c tree.c tree-walk.c unpack-trees.c upload-pack.c url.c
	urlmatch.c usage.c userdiff.c utf8.c varint.c version.c versioncmp.c walker.c wildmatch.c
	worktree.c wrapper.c write-or-die.c ws.c wt-status.c xdiff-interface.c
	zlib.c)

add_library(libgit ${libgit_SOURCES} ${compat_SOURCES})

set(libxdiff_SOURCES
	xdiff/xdiffi.c xdiff/xprepare.c xdiff/xutils.c xdiff/xemit.c
	xdiff/xmerge.c xdiff/xpatience.c xdiff/xhistogram.c)
add_library(xdiff STATIC ${libxdiff_SOURCES})

set(libvcs-svn_SOURCES
	vcs-svn/line_buffer.c vcs-svn/sliding_window.c vcs-svn/fast_export.c
	vcs-svn/svndiff.c vcs-svn/svndump.c)
add_library(vcs-svn STATIC ${libvcs-svn_SOURCES})

if(WIN32)
	if(NOT MSVC)#use windres when compiling with gcc and clang
		add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/git.res
				COMMAND ${WINDRES_EXE} -O coff -DMAJOR=${PROJECT_VERSION_MAJOR} -DMINOR=${PROJECT_VERSION_MINOR}
					-DMICRO=${PROJECT_VERSION_PATCH} -DPATCHLEVEL=0 -DGIT_VERSION="\\\"${PROJECT_VERSION}.GIT\\\""
					-i ${CMAKE_SOURCE_DIR}/git.rc -o ${CMAKE_BINARY_DIR}/git.res
				WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
				VERBATIM)
	else()#MSVC use rc
		add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/git.res
				COMMAND ${CMAKE_RC_COMPILER} /d MAJOR=${PROJECT_VERSION_MAJOR} /d MINOR=${PROJECT_VERSION_MINOR}
					/d MICRO=${PROJECT_VERSION_PATCH} /d PATCHLEVEL=0 /d GIT_VERSION="${PROJECT_VERSION}.GIT"
					/fo ${CMAKE_BINARY_DIR}/git.res ${CMAKE_SOURCE_DIR}/git.rc
				WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
				VERBATIM)
	endif()
	add_custom_target(git-rc DEPENDS ${CMAKE_BINARY_DIR}/git.res)
endif()

#link all required libraries to common-main
add_library(common-main OBJECT common-main.c)

target_link_libraries(common-main libgit xdiff ${ZLIB_LIBRARIES})
if(Intl_FOUND)
	target_link_libraries(common-main ${Intl_LIBRARIES})
endif()
if(Iconv_FOUND)
	target_link_libraries(common-main ${Iconv_LIBRARIES})
endif()
if(WIN32)
	target_link_libraries(common-main ws2_32 ntdll ${CMAKE_BINARY_DIR}/git.res)
	add_dependencies(common-main git-rc)
	if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
		target_link_options(common-main PUBLIC -municode -Wl,--nxcompat -Wl,--dynamicbase -Wl,--pic-executable,-e,mainCRTStartup)
	elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
		target_link_options(common-main PUBLIC -municode -Wl,-nxcompat -Wl,-dynamicbase -Wl,-entry:wmainCRTStartup -Wl,invalidcontinue.obj)
	elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
		target_link_options(common-main PUBLIC /IGNORE:4217 /IGNORE:4049 /NOLOGO /ENTRY:wmainCRTStartup /SUBSYSTEM:CONSOLE invalidcontinue.obj)
	endif()
elseif(UNIX)
	target_link_libraries(common-main pthread rt)
endif()


set(git_SOURCES
	builtin/add.c builtin/am.c builtin/annotate.c builtin/apply.c
	builtin/archive.c builtin/bisect--helper.c builtin/blame.c
	builtin/branch.c builtin/bundle.c builtin/cat-file.c builtin/check-attr.c
	builtin/check-ignore.c builtin/check-mailmap.c builtin/check-ref-format.c
	builtin/checkout-index.c builtin/checkout.c builtin/clean.c
	builtin/clone.c builtin/column.c builtin/commit-tree.c
	builtin/commit.c builtin/commit-graph.c builtin/config.c
	builtin/count-objects.c builtin/credential.c builtin/describe.c
	builtin/diff-files.c builtin/diff-index.c builtin/diff-tree.c
	builtin/diff.c builtin/difftool.c builtin/env--helper.c
	builtin/fast-export.c builtin/fetch-pack.c builtin/fetch.c builtin/fmt-merge-msg.c
	builtin/for-each-ref.c builtin/fsck.c builtin/gc.c
	builtin/get-tar-commit-id.c builtin/grep.c builtin/hash-object.c
	builtin/help.c builtin/index-pack.c builtin/init-db.c
	builtin/interpret-trailers.c builtin/log.c builtin/ls-files.c
	builtin/ls-remote.c builtin/ls-tree.c builtin/mailinfo.c builtin/mailsplit.c
	builtin/merge.c builtin/merge-base.c builtin/merge-file.c builtin/merge-index.c
	builtin/merge-ours.c builtin/merge-recursive.c builtin/merge-tree.c
	builtin/mktag.c builtin/mktree.c builtin/multi-pack-index.c builtin/mv.c
	builtin/name-rev.c builtin/notes.c builtin/pack-objects.c builtin/pack-redundant.c
	builtin/pack-refs.c builtin/patch-id.c builtin/prune-packed.c builtin/prune.c
	builtin/pull.c builtin/push.c builtin/range-diff.c builtin/read-tree.c
	builtin/rebase.c builtin/receive-pack.c builtin/reflog.c builtin/remote.c
	builtin/remote-ext.c builtin/remote-fd.c builtin/repack.c builtin/replace.c
	builtin/rerere.c builtin/reset.c builtin/rev-list.c builtin/rev-parse.c
	builtin/revert.c builtin/rm.c builtin/send-pack.c builtin/shortlog.c
	builtin/show-branch.c builtin/show-index.c builtin/show-ref.c
	builtin/sparse-checkout.c builtin/stash.c builtin/stripspace.c
	builtin/submodule--helper.c builtin/symbolic-ref.c builtin/tag.c
	builtin/unpack-file.c builtin/unpack-objects.c builtin/update-index.c
	builtin/update-ref.c builtin/update-server-info.c builtin/upload-archive.c
	builtin/upload-pack.c builtin/var.c builtin/verify-commit.c builtin/verify-pack.c
	builtin/verify-tag.c builtin/worktree.c builtin/write-tree.c)

add_executable(git git.c ${git_SOURCES})
target_link_libraries(git common-main )

add_executable(git-bugreport bugreport.c)
target_link_libraries(git-bugreport common-main)

add_executable(git-credential-store credential-store.c)
target_link_libraries(git-credential-store common-main)

add_executable(git-daemon daemon.c)
target_link_libraries(git-daemon common-main)

add_executable(git-fast-import fast-import.c)
target_link_libraries(git-fast-import common-main)

add_executable(git-http-backend http-backend.c)
target_link_libraries(git-http-backend common-main)

add_executable(git-sh-i18n--envsubst sh-i18n--envsubst.c)
target_link_libraries(git-sh-i18n--envsubst common-main)

add_executable(git-shell shell.c)
target_link_libraries(git-shell common-main)

if(CURL_FOUND)
	add_library(http_obj OBJECT http.c)

	add_executable(git-imap-send imap-send.c)
	target_link_libraries(git-imap-send http_obj common-main ${CURL_LIBRARIES})

	add_executable(git-http-fetch http-walker.c http-fetch.c)
	target_link_libraries(git-http-fetch http_obj common-main ${CURL_LIBRARIES})

	add_executable(git-remote-http http-walker.c remote-curl.c)
	target_link_libraries(git-remote-http http_obj common-main ${CURL_LIBRARIES} )

	if(EXPAT_FOUND)
		add_executable(git-http-push http-push.c)
		target_link_libraries(git-http-push http_obj common-main ${CURL_LIBRARIES} ${EXPAT_LIBRARIES})
	endif()
endif()

add_executable(git-remote-testsvn remote-testsvn.c)
target_link_libraries(git-remote-testsvn common-main vcs-svn)

if(NOT NO_UNIX_SOCKETS)
	add_executable(git-credential-cache credential-cache.c)
	target_link_libraries(git-credential-cache common-main)

	add_executable(git-credential-cache--daemon credential-cache--daemon.c)
	target_link_libraries(git-credential-cache--daemon common-main)
endif()


set(git_builtin_extra
	cherry cherry-pick format-patch fsck-objects
	init merge-subtree restore show
	stage status switch whatchanged)

#Creating hardlinks
foreach(s ${git_SOURCES} ${git_builtin_extra})
	string(REPLACE "builtin/" "" s ${s})
	string(REPLACE ".c" "" s ${s})
	file(APPEND ${CMAKE_BINARY_DIR}/CreateLinks.cmake "file(CREATE_LINK git${EXE_EXTENSION} git-${s}${EXE_EXTENSION})\n")
	list(APPEND git_links ${CMAKE_BINARY_DIR}/git-${s}${EXE_EXTENSION})
endforeach()

if(CURL_FOUND)
	set(remote_exes
		git-remote-https git-remote-ftp git-remote-ftps)
	foreach(s ${remote_exes})
		file(APPEND ${CMAKE_BINARY_DIR}/CreateLinks.cmake "file(CREATE_LINK git-remote-http${EXE_EXTENSION} ${s}${EXE_EXTENSION})\n")
		list(APPEND git_http_links ${CMAKE_BINARY_DIR}/${s}${EXE_EXTENSION})
	endforeach()
endif()

add_custom_command(OUTPUT ${git_links} ${git_http_links}
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/CreateLinks.cmake
		DEPENDS git git-remote-http)
add_custom_target(git-links ALL DEPENDS ${git_links} ${git_http_links})


#creating required scripts
set(SHELL_PATH /bin/sh)
set(PERL_PATH /usr/bin/perl)
set(LOCALEDIR ${FALLBACK_RUNTIME_PREFIX}/share/locale)
set(GITWEBDIR ${FALLBACK_RUNTIME_PREFIX}/share/locale)
set(INSTLIBDIR ${FALLBACK_RUNTIME_PREFIX}/share/perl5)

#shell scripts
set(git_shell_scripts
	git-bisect git-difftool--helper git-filter-branch
	git-merge-octopus git-merge-one-file git-merge-resolve
	git-mergetool git-quiltimport
	git-request-pull git-submodule git-web--browse
	git-mergetool--lib git-parse-remote git-rebase--preserve-merges
	git-sh-setup git-sh-i18n git-instaweb)

foreach(script ${git_shell_scripts})
	file(STRINGS ${CMAKE_SOURCE_DIR}/${script}.sh content NEWLINE_CONSUME)
	string(REPLACE "@SHELL_PATH@" "${SHELL_PATH}" content "${content}")
	string(REPLACE "@@DIFF@@" "diff" content "${content}")
	string(REPLACE "@LOCALEDIR@" "${LOCALEDIR}" content "${content}")
	string(REPLACE "@GITWEBDIR@" "${GITWEBDIR}" content "${content}")
	string(REPLACE "@@NO_CURL@@" "" content "${content}")
	string(REPLACE "@@USE_GETTEXT_SCHEME@@" "" content "${content}")
	string(REPLACE "# @@BROKEN_PATH_FIX@@" "" content "${content}")
	string(REPLACE "@@PERL@@" "${PERL_PATH}" content "${content}")
	string(REPLACE "@@SANE_TEXT_GREP@@" "-a" content "${content}")
	string(REPLACE "@@PAGER_ENV@@" "LESS=FRX LV=-c" content "${content}")
	file(WRITE ${CMAKE_BINARY_DIR}/${script} ${content})
endforeach()

#perl scripts
set(git_perl_scripts
	git-add--interactive git-archimport git-cvsexportcommit
	git-cvsimport git-cvsserver git-send-email git-svn)

#create perl header
file(STRINGS ${CMAKE_SOURCE_DIR}/perl/header_templates/fixed_prefix.template.pl perl_header )
string(REPLACE "@@PATHSEP@@" ":" perl_header "${perl_header}")
string(REPLACE "@@INSTLIBDIR@@" "${INSTLIBDIR}" perl_header "${perl_header}")

foreach(script ${git_perl_scripts})
	file(STRINGS ${CMAKE_SOURCE_DIR}/${script}.perl content NEWLINE_CONSUME)
	string(REPLACE "#!/usr/bin/perl" "#!/usr/bin/perl\n${perl_header}\n" content "${content}")
	string(REPLACE "@@GIT_VERSION@@" "${PROJECT_VERSION}" content "${content}")
	file(WRITE ${CMAKE_BINARY_DIR}/${script} ${content})
endforeach()

#python script
file(STRINGS ${CMAKE_SOURCE_DIR}/git-p4.py content NEWLINE_CONSUME)
string(REPLACE "#!/usr/bin/env python" "#!/usr/bin/python" content "${content}")
file(WRITE ${CMAKE_BINARY_DIR}/git-p4 ${content})

#perl modules
file(GLOB_RECURSE perl_modules "${CMAKE_SOURCE_DIR}/perl/*.pm")

foreach(pm ${perl_modules})
	string(REPLACE "${CMAKE_SOURCE_DIR}/perl/" "" file_path ${pm})
	file(STRINGS ${pm} content NEWLINE_CONSUME)
	string(REPLACE "@@LOCALEDIR@@" "${LOCALEDIR}" content "${content}")
	string(REPLACE "@@NO_PERL_CPAN_FALLBACKS@@" "" content "${content}")
	file(WRITE ${CMAKE_BINARY_DIR}/perl/build/lib/${file_path} ${content})
#test-lib.sh requires perl/build/lib to be the build directory of perl modules
endforeach()


#templates
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/templates/blt/branches) #create branches
set(hooks_templates
	applypatch-msg.sample pre-applypatch.sample pre-push.sample
	commit-msg.sample pre-commit.sample pre-rebase.sample
	fsmonitor-watchman.sample pre-merge-commit.sample pre-receive.sample
	post-update.sample prepare-commit-msg.sample update.sample)

#templates have @.*@ replacement so use configure_file instead
#hooks
foreach(tm ${hooks_templates})
	configure_file(${CMAKE_SOURCE_DIR}/templates/hooks--${tm} ${CMAKE_BINARY_DIR}/templates/blt/hooks/${tm} @ONLY)
endforeach()

#info
configure_file(${CMAKE_SOURCE_DIR}/templates/info--exclude ${CMAKE_BINARY_DIR}/templates/blt/info/exclude @ONLY)

#this
configure_file(${CMAKE_SOURCE_DIR}/templates/this--description ${CMAKE_BINARY_DIR}/templates/blt/description @ONLY)


#translations
if(MSGFMT_EXE)
	set(po_files bg  ca  de  el  es  fr  is  it  ko  pt_PT  ru  sv  tr  vi  zh_CN  zh_TW)
	foreach(po ${po_files})
		file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/po/build/locale/${po}/LC_MESSAGES)
		add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/po/build/locale/${po}/LC_MESSAGES/git.mo
				COMMAND ${MSGFMT_EXE} --check --statistics -o ${CMAKE_BINARY_DIR}/po/build/locale/${po}/LC_MESSAGES/git.mo ${CMAKE_SOURCE_DIR}/po/${po}.po)
		list(APPEND po_gen ${CMAKE_BINARY_DIR}/po/build/locale/${po}/LC_MESSAGES/git.mo)
	endforeach()
	add_custom_target(po-gen ALL DEPENDS ${po_gen})
endif()


#to help with the install
list(TRANSFORM git_shell_scripts PREPEND "${CMAKE_BINARY_DIR}/")
list(TRANSFORM git_perl_scripts PREPEND "${CMAKE_BINARY_DIR}/")

#install
install(TARGETS git git-shell
	RUNTIME DESTINATION bin)
install(PROGRAMS ${CMAKE_BINARY_DIR}/git-cvsserver
	DESTINATION bin)

list(REMOVE_ITEM PROGRAMS_BUILT git git-shell)
install(TARGETS ${PROGRAMS_BUILT}
	RUNTIME DESTINATION libexec/git-core)

set(bin_links
	git-receive-pack git-upload-archive git-upload-pack)

foreach(b ${bin_links})
install(CODE "file(CREATE_LINK ${CMAKE_INSTALL_PREFIX}/bin/git${EXE_EXTENSION} ${CMAKE_INSTALL_PREFIX}/bin/${b}${EXE_EXTENSION})")
endforeach()

install(CODE "file(CREATE_LINK ${CMAKE_INSTALL_PREFIX}/bin/git${EXE_EXTENSION} ${CMAKE_INSTALL_PREFIX}/libexec/git-core/git${EXE_EXTENSION})")
install(CODE "file(CREATE_LINK ${CMAKE_INSTALL_PREFIX}/bin/git-shell${EXE_EXTENSION} ${CMAKE_INSTALL_PREFIX}/libexec/git-core/git-shell${EXE_EXTENSION})")

foreach(b ${git_links})
	string(REPLACE "${CMAKE_BINARY_DIR}" "" b ${b})
	install(CODE "file(CREATE_LINK ${CMAKE_INSTALL_PREFIX}/bin/git${EXE_EXTENSION} ${CMAKE_INSTALL_PREFIX}/libexec/git-core/${b}${EXE_EXTENSION})")
endforeach()

foreach(b ${git_http_links})
	string(REPLACE "${CMAKE_BINARY_DIR}" "" b ${b})
	install(CODE "file(CREATE_LINK  ${CMAKE_INSTALL_PREFIX}/libexec/git-core/git-remote-http${EXE_EXTENSION} ${CMAKE_INSTALL_PREFIX}/libexec/git-core/${b}${EXE_EXTENSION})")
endforeach()

install(PROGRAMS ${git_shell_scripts} ${git_perl_scripts} ${CMAKE_BINARY_DIR}/git-p4
	DESTINATION libexec/git-core)

install(DIRECTORY mergetools DESTINATION libexec/git-core)
install(DIRECTORY ${CMAKE_BINARY_DIR}/perl/build/lib/ DESTINATION share/perl5
	FILES_MATCHING PATTERN "*.pm")
install(DIRECTORY ${CMAKE_BINARY_DIR}/templates/blt/ DESTINATION share/git-core/templates)

if(MSGFMT_EXE)
	install(DIRECTORY ${CMAKE_BINARY_DIR}/po/build/locale DESTINATION share)
endif()


if(BUILD_TESTING)

#tests-helpers
add_executable(test-fake-ssh t/helper/test-fake-ssh.c)
target_link_libraries(test-fake-ssh common-main)

add_executable(test-line-buffer t/helper/test-line-buffer.c)
target_link_libraries(test-line-buffer common-main vcs-svn)

add_executable(test-svn-fe t/helper/test-svn-fe.c)
target_link_libraries(test-svn-fe common-main vcs-svn)

set(test-tool_SOURCES
	t/helper/test-tool.c t/helper/test-advise.c t/helper/test-bloom.c t/helper/test-chmtime.c
	t/helper/test-config.c t/helper/test-ctype.c t/helper/test-date.c t/helper/test-delta.c
	t/helper/test-dir-iterator.c t/helper/test-drop-caches.c t/helper/test-dump-cache-tree.c
	t/helper/test-dump-fsmonitor.c t/helper/test-dump-split-index.c
	t/helper/test-dump-untracked-cache.c t/helper/test-example-decorate.c
	t/helper/test-genrandom.c t/helper/test-genzeros.c t/helper/test-hash.c
	t/helper/test-hashmap.c t/helper/test-hash-speed.c t/helper/test-index-version.c
	t/helper/test-json-writer.c t/helper/test-lazy-init-name-hash.c
	t/helper/test-match-trees.c t/helper/test-mergesort.c t/helper/test-mktemp.c
	t/helper/test-oidmap.c t/helper/test-online-cpus.c t/helper/test-parse-options.c
	t/helper/test-parse-pathspec-file.c t/helper/test-path-utils.c t/helper/test-pkt-line.c
	t/helper/test-prio-queue.c t/helper/test-progress.c t/helper/test-reach.c
	t/helper/test-read-cache.c t/helper/test-read-graph.c t/helper/test-read-midx.c
	t/helper/test-ref-store.c t/helper/test-regex.c t/helper/test-repository.c
	t/helper/test-revision-walking.c t/helper/test-run-command.c t/helper/test-scrap-cache-tree.c
	t/helper/test-serve-v2.c t/helper/test-sha1.c t/helper/test-oid-array.c t/helper/test-sha256.c
	t/helper/test-sigchain.c t/helper/test-strcmp-offset.c t/helper/test-string-list.c
	t/helper/test-submodule-config.c t/helper/test-submodule-nested-repo-config.c t/helper/test-subprocess.c
	t/helper/test-trace2.c t/helper/test-urlmatch-normalization.c t/helper/test-xml-encode.c
	t/helper/test-wildmatch.c t/helper/test-windows-named-pipe.c t/helper/test-write-cache.c)

add_executable(test-tool ${test-tool_SOURCES})
target_link_libraries(test-tool common-main)

set_target_properties(test-fake-ssh test-line-buffer test-svn-fe test-tool
			PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/t/helper)

if(MSVC)
	set_target_properties(test-fake-ssh test-line-buffer test-svn-fe test-tool
				PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/t/helper)
	set_target_properties(test-fake-ssh test-line-buffer test-svn-fe test-tool
				PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/t/helper)
endif()

#wrapper scripts
set(wrapper_scripts
	git git-upload-pack git-receive-pack git-upload-archive git-shell git-remote-ext)

set(wrapper_test_scripts
	test-fake-ssh test-line-buffer test-svn-fe test-tool)


foreach(script ${wrapper_scripts})
	file(STRINGS ${CMAKE_SOURCE_DIR}/wrap-for-bin.sh content NEWLINE_CONSUME)
	string(REPLACE "@@BUILD_DIR@@" "${CMAKE_BINARY_DIR}" content "${content}")
	string(REPLACE "@@PROG@@" "${script}${EXE_EXTENSION}" content "${content}")
	file(WRITE ${CMAKE_BINARY_DIR}/bin-wrappers/${script} ${content})
endforeach()

foreach(script ${wrapper_test_scripts})
	file(STRINGS ${CMAKE_SOURCE_DIR}/wrap-for-bin.sh content NEWLINE_CONSUME)
	string(REPLACE "@@BUILD_DIR@@" "${CMAKE_BINARY_DIR}" content "${content}")
	string(REPLACE "@@PROG@@" "t/helper/${script}${EXE_EXTENSION}" content "${content}")
	file(WRITE ${CMAKE_BINARY_DIR}/bin-wrappers/${script} ${content})
endforeach()

file(STRINGS ${CMAKE_SOURCE_DIR}/wrap-for-bin.sh content NEWLINE_CONSUME)
string(REPLACE "@@BUILD_DIR@@" "${CMAKE_BINARY_DIR}" content "${content}")
string(REPLACE "@@PROG@@" "git-cvsserver" content "${content}")
file(WRITE ${CMAKE_BINARY_DIR}/bin-wrappers/git-cvsserver ${content})

#options for configuring test options
option(PERL_TESTS "Perform tests that use perl" ON)
option(PYTHON_TESTS "Perform tests that use python" ON)

#GIT-BUILD-OPTIONS
set(TEST_SHELL_PATH ${SHELL_PATH})
set(DIFF diff)
set(PYTHON_PATH /usr/bin/python)
set(TAR tar)
set(NO_CURL )
set(NO_EXPAT )
set(USE_LIBPCRE1 )
set(USE_LIBPCRE2 )
set(NO_LIBPCRE1_JIT )
set(NO_PERL )
set(NO_PTHREADS )
set(NO_PYTHON )
set(PAGER_ENV "LESS=FRX LV=-c")
set(DC_SHA1 YesPlease)
set(RUNTIME_PREFIX true)
set(NO_GETTEXT )

if(NOT CURL_FOUND)
	set(NO_CURL 1)
endif()

if(NOT EXPAT_FOUND)
	set(NO_EXPAT 1)
endif()

if(NOT Intl_FOUND)
	set(NO_GETTEXT 1)
endif()

if(NOT PERL_TESTS)
	set(NO_PERL 1)
endif()

if(NOT PYTHON_TESTS)
	set(NO_PYTHON 1)
endif()

file(WRITE ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "SHELL_PATH='${SHELL_PATH}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "TEST_SHELL_PATH='${TEST_SHELL_PATH}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "PERL_PATH='${PERL_PATH}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "DIFF='${DIFF}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "PYTHON_PATH='${PYTHON_PATH}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "TAR='${TAR}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "NO_CURL='${NO_CURL}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "NO_EXPAT='${NO_EXPAT}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "USE_LIBPCRE1='${USE_LIBPCRE1}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "NO_LIBPCRE1_JIT='${NO_LIBPCRE1_JIT}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "NO_PERL='${NO_PERL}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "NO_PTHREADS='${NO_PTHREADS}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "NO_UNIX_SOCKETS='${NO_UNIX_SOCKETS}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "PAGER_ENV='${PAGER_ENV}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "DC_SHA1='${DC_SHA1}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "X='${EXE_EXTENSION}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "NO_GETTEXT='${NO_GETTEXT}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "RUNTIME_PREFIX='${RUNTIME_PREFIX}'\n")
file(APPEND ${CMAKE_BINARY_DIR}/GIT-BUILD-OPTIONS "NO_PYTHON='${NO_PYTHON}'\n")

#Make the tests work when building out of the source tree
if(NOT ${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
	file(RELATIVE_PATH BUILD_DIR_RELATIVE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/CMakeCache.txt)
	string(REPLACE "/CMakeCache.txt" "" BUILD_DIR_RELATIVE ${BUILD_DIR_RELATIVE})
	#Setting the build directory in test-lib.sh before running tests
	file(WRITE ${CMAKE_BINARY_DIR}/CTestCustom.cmake
		"file(STRINGS ${CMAKE_SOURCE_DIR}/t/test-lib.sh GIT_BUILD_DIR_REPL REGEX \"GIT_BUILD_DIR=(.*)\")\n"
		"file(STRINGS ${CMAKE_SOURCE_DIR}/t/test-lib.sh content NEWLINE_CONSUME)\n"
		"string(REPLACE \"\${GIT_BUILD_DIR_REPL}\" \"GIT_BUILD_DIR=\\\"$TEST_DIRECTORY\\\"/../${BUILD_DIR_RELATIVE}\" content \"\${content}\")\n"
		"file(WRITE ${CMAKE_SOURCE_DIR}/t/test-lib.sh \${content})")
	#misc copies
	file(COPY ${CMAKE_SOURCE_DIR}/t/chainlint.sed DESTINATION ${CMAKE_BINARY_DIR}/t/)
	file(COPY ${CMAKE_SOURCE_DIR}/po/is.po DESTINATION ${CMAKE_BINARY_DIR}/po/)
	file(COPY ${CMAKE_SOURCE_DIR}/mergetools/tkdiff DESTINATION ${CMAKE_BINARY_DIR}/mergetools/)
	file(COPY ${CMAKE_SOURCE_DIR}/contrib/completion/git-prompt.sh DESTINATION ${CMAKE_BINARY_DIR}/contrib/completion/)
	file(COPY ${CMAKE_SOURCE_DIR}/contrib/completion/git-completion.bash DESTINATION ${CMAKE_BINARY_DIR}/contrib/completion/)
	file(COPY ${CMAKE_SOURCE_DIR}/contrib/svn-fe/svnrdump_sim.py DESTINATION ${CMAKE_BINARY_DIR}/contrib/svn-fe/)
endif()

file(GLOB test_scipts "${CMAKE_SOURCE_DIR}/t/t[0-9]*.sh")

#test
foreach(tsh ${test_scipts})
	add_test(NAME ${tsh}
		COMMAND ${SH_EXE} ${tsh}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/t)
endforeach()

endif()#BUILD_TESTING