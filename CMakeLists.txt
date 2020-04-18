#
#	Copyright (c) 2020 Sibi Siddharthan
#

cmake_minimum_required(VERSION 3.14)

#Parse GIT-VERSION-GEN to get the version
file(STRINGS ${CMAKE_SOURCE_DIR}/GIT-VERSION-GEN git_version REGEX "DEF_VER=v(.*)")
string(REPLACE "DEF_VER=v" "" git_version ${git_version})
string(REPLACE ".GIT" ".0" git_version ${git_version})#for building from a snapshot

project(git
	VERSION ${git_version}
	LANGUAGES C)


include(CheckTypeSize)
include(CheckCSourceRuns)
include(CheckCSourceCompiles)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckStructHasMember)

find_package(ZLIB REQUIRED)
find_package(CURL)
find_package(EXPAT)
find_package(Iconv)
find_package(Intl)

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

set(FALLBACK_RUNTIME_PREFIX /home/$ENV{USER})
add_compile_definitions(FALLBACK_RUNTIME_PREFIX="${FALLBACK_RUNTIME_PREFIX}")

add_compile_definitions(PROCFS_EXECUTABLE_PATH="/proc/self/exe" HAVE_DEV_TTY )
list(APPEND compat_SOURCES unix-socket.c)

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
	setenv  mkdtemp poll pread  memmem unsetenv hstrerror)

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

#programs
set(PROGRAMS_BUILT
	git git-bugreport git-credential-store git-daemon git-fast-import git-http-backend git-sh-i18n--envsubst
	git-shell git-remote-testsvn git-credential-cache git-credential-cache--daemon)

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

#link all required libraries to common-main
add_library(common-main OBJECT common-main.c)
target_link_libraries(common-main libgit xdiff ${ZLIB_LIBRARIES} pthread rt)
if(Intl_FOUND)
	target_link_libraries(common-main ${Intl_LIBRARIES})
endif()
if(Iconv_FOUND)
	target_link_libraries(common-main ${Iconv_LIBRARIES})
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

add_executable(git-credential-cache credential-cache.c)
target_link_libraries(git-credential-cache common-main)

add_executable(git-credential-cache--daemon credential-cache--daemon.c)
target_link_libraries(git-credential-cache--daemon common-main)


set(git_builtin_extra
	cherry cherry-pick format-patch fsck-objects
	init merge-subtree restore show
	stage status switch whatchanged)

#Creating hardlinks
foreach(s ${git_SOURCES} ${git_builtin_extra})
	string(REPLACE "builtin/" "" s ${s})
	string(REPLACE ".c" "" s ${s})
	file(APPEND ${CMAKE_BINARY_DIR}/CreateLinks.cmake "file(CREATE_LINK git git-${s})\n")
	list(APPEND git_links ${CMAKE_BINARY_DIR}/git-${s})
endforeach()

if(CURL_FOUND)
	set(remote_exes
		git-remote-https git-remote-ftp git-remote-ftps)
	foreach(s ${remote_exes})
		file(APPEND ${CMAKE_BINARY_DIR}/CreateLinks.cmake "file(CREATE_LINK git-remote-http ${s})\n")
		list(APPEND git_http_links ${CMAKE_BINARY_DIR}/${s})
	endforeach()
endif()

add_custom_command(OUTPUT ${git_links} ${git_http_links}
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/CreateLinks.cmake
		DEPENDS git git-remote-http)
add_custom_target(git-links ALL DEPENDS ${git_links} ${git_http_links})