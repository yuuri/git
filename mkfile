</sys/src/ape/config

CFILES=`{sed -n '/^BUILTIN_OBJS *\+= *(.*)\.o$/s//\1.c/p' Makefile}
OFILES=\
	${CFILES:%.c=%.$O}\
	common-main.$O\

X_CFILES=`{sed -n '/^BUILT_INS *\+= *git-(.*)\$X$/s//\1/p' Makefile}

#APP_CFILES=`{sed -n '/^PROGRAM_OBJS *\+= *(.*)\.o$/s//\1.c/p' Makefile}
APP_CFILES=\
	credential-store.c\
	http-fetch.c\
	http-push.c\

TARG=\
	git\
	${CFILES:builtin/%.c=git-%}\
	${X_CFILES:%=git-%}\
	${APP_CFILES:%.c=git-%}\
	git-remote-http\
	git-remote-https\
	git-remote-ftp\
	git-remote-ftps\

# NO_UNIXSOCK
#	git-credential-cache\
#	git-credential-cache--daemon\

#GIT_HOST_CPU=i386|i686|x86_64

ROOT=`{pwd}
<GIT-VERSION-FILE
COMMIT_HASH=`{git rev-parse -q --verify HEAD}

# -w flag isn't intentionally set because it is noisy.
# -T flag isn't intentionally set.
CFLAGS=-FVB+ -c\
	-I$ROOT\
	-I$ROOT/compat/plan9\
	-I$ROOT/compat/regex\
	-D__PLAN9__\
	-D_POSIX_SOURCE\
	-D_BSD_EXTENSION\
	-D_SUSV2_SOURCE\
	-D_PLAN9_SOURCE\
	-D_RESEARCH_SOURCE\
	-D_REENTRANT_SOURCE\
	-DHAVE_SOCK_OPTS\
	-DNO_NSEC\
	-DNO_SYMLINK_HEAD\
	-DNO_GECOS_IN_PWENT\
	-DNO_GETTEXT\
	-DNO_STRCASESTR\
	-DNO_STRLCPY\
	-DNO_STRTOUMAX\
	-DNO_MBSUPPORT\
	-DNO_MKDTEMP\
	-DNO_UNSETENV\
	-DNEEDS_SYS_PARAM_H\
	-DNO_INITGROUPS\
	-DNO_MMAP\
	-DNO_ST_BLOCKS_IN_STRUCT_STAT\
	-DNO_STRUCT_ITIMERVAL\
	-DNO_SETITIMER\
	-Dsockaddr_storage=sockaddr_in6\
	-DNO_UNIX_SOCKETS\
	-DNO_ICONV\
	-DSHA1_OPENSSL\
	-DSHA256_OPENSSL\
	-DNO_MEMMEM\
	-DHAVE_STDBOOL_H\
	-DHAVE_STDINT_H\
	-DHAVE_LOCALE_H\
	-DHAVE_CLOCK_GETTIME\
	-DGIT_VERSION="$GIT_VERSION"\
	-DGIT_BUILT_FROM_COMMIT="$COMMIT_HASH"\
	-DGIT_USER_AGENT="git/$GIT_VERSION"\
	-DETC_GITCONFIG="/sys/lib/git/config"\
	-DETC_GITATTRIBUTES="/sys/lib/git/attributes"\
	-DUSER_GITCONFIG="~/lib/git/config"\
	-DUSER_GITCREDENTIALS="~/lib/git/credentials"\
	-DUSER_GITCREDENTIAL_CACHE="~/lib/git/credential-cache"\
	-DDEFAULT_GIT_TEMPLATE_DIR="/sys/lib/git/templates"\
	-DGIT_HOST_CPU="i386"\
	-DGIT_EXEC_PATH="/bin/git-core"\
	-DGIT_MAN_PATH="/sys/man"\
	-DGIT_INFO_PATH=""\
	-DGIT_HTML_PATH=""\
	-DFALLBACK_RUNTIME_PREFIX="/bin/git-core"\
	-DDEFAULT_PAGER="/bin/p"\
	-DPAGER_ENV="terminal="\
	-DHOME_ENVIRONMENT="home"\
	-DPATH_ENVIRONMENT="path"\
	-D_PATH_SEP=1\
	-D_PATH_DEFPATH="/bin"\
	-DSHELL_PATH="/bin/rc"\

LIB_CFILES=`{sed -n '/^LIB_OBJS *\+= *(.*)\.o$/s//\1.c/p' Makefile}
LIB_OFILES=\
	${LIB_CFILES:%.c=%.$O}\
	compat/qsort_s.$O\
	compat/strcasestr.$O\
	compat/strlcpy.$O\
	compat/strtoumax.$O\
	compat/strtoimax.$O\
	compat/setenv.$O\
	compat/mkdtemp.$O\
	compat/unsetenv.$O\
	compat/mmap.$O\
	compat/memmem.$O\
	compat/regex/regex.$O\

XDIFF_CFILES=`{sed -n '/^XDIFF_OBJS *\+= *(.*)\.o$/s//\1.c/p' Makefile}
XDIFF_OFILES=${XDIFF_CFILES:%.c=%.$O}

HFILES=\
	`{ls *.h}\
	`{ls */*.h}\
	`{ls */*/*.h}\
	`{ls */*/*/*.h}\
	command-list.h\

BIN=/$objtype/bin/git-core
OBJBIN=/$objtype/bin

LIB=\
	libgit.a$O\
	xdiff/lib.a$O\
	/$objtype/lib/ape/libcurl.a\
	/$objtype/lib/ape/libssl.a\
	/$objtype/lib/ape/libcrypto.a\
	/$objtype/lib/ape/libexpat.a\
	/$objtype/lib/ape/libz.a\

CLEANFILES=command-list.h

</sys/src/cmd/mkmany

LIBGIT=libgit.a$O
LIBGITOBJ=${LIB_OFILES:%=$LIBGIT(%)}
LIBXDIFF=xdiff/lib.a$O
LIBXDIFFOBJ=${XDIFF_OFILES:%=$LIBXDIFF(%)}

command-list.h:D:	command-list.txt
	rc ./generate-cmdlist.rc $prereq >$target

${CFILES:builtin/%.c=$O.git-%} ${X_CFILES:%=$O.git-%}:D:	plan9/wrap.c
	for(i in $target)
		$CC -FTVw -o $i $prereq

$O.git-credential-store:	credential-store.$O common-main.$O $LIB
	$LD $LDFLAGS -o $target $prereq

$O.git-http-fetch:	http-fetch.$O http.$O http-walker.$O common-main.$O $LIB
	$LD $LDFLAGS -o $target $prereq

$O.git-http-push:	http-push.$O http.$O common-main.$O $LIB
	$LD $LDFLAGS -o $target $prereq

git-http-%.$O:	http-%.c
	$CC $CFLAGS -o $target $prereq

$O.git-remote-http:	remote-curl.$O http.$O http-walker.$O common-main.$O $LIB
	$LD $LDFLAGS -o $target $prereq

$O.git-remote-https:	remote-curl.$O http.$O http-walker.$O common-main.$O $LIB
	$LD $LDFLAGS -o $target $prereq

$O.git-remote-ftp:	remote-curl.$O http.$O http-walker.$O common-main.$O $LIB
	$LD $LDFLAGS -o $target $prereq

$O.git-remote-ftps:	remote-curl.$O http.$O http-walker.$O common-main.$O $LIB
	$LD $LDFLAGS -o $target $prereq

# git should be copied into both $BIN and $OBJBIN
git.install:V:	$O.git
	cp $O.git $OBJBIN/git

$LIBGIT:	$LIBGITOBJ
	ar vu $target $newmember

$LIBXDIFF:	$LIBXDIFFOBJ
	ar vu $target $newmember

%.$O:	%.c
	$CC $CFLAGS -o $target $stem.c

$LIBGIT(%.$O):N:	%.$O

$LIBXDIFF(%.$O):N:	%.$O

clean:V:
	rm -f *.[$OS] [$OS].out y.tab.? lex.yy.c y.debug y.output $CLEANFILES
	rm -f */*.[$OS] */*/*.[$OS]
