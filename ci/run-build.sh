#!/bin/sh
#
# Build Git
#

. ${0%/*}/lib-travisci.sh

if [ "$jobname" = linux-gcc ]; then
	gcc-6 --version
	cat >config.mak <<-EOF
	CC=gcc-6
	CFLAGS = -g -O2 -Wall
	CFLAGS += -Wextra
	CFLAGS += -Wmissing-prototypes
	CFLAGS += -Wno-empty-body
	CFLAGS += -Wno-maybe-uninitialized
	CFLAGS += -Wno-missing-field-initializers
	CFLAGS += -Wno-sign-compare
	CFLAGS += -Wno-unused-function
	CFLAGS += -Wno-unused-parameter
	EOF
fi
make --jobs=2
