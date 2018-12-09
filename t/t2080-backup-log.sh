#!/bin/sh

test_description='backup-log'

. ./test-lib.sh

test_expect_success 'setup' '
	test_commit initial &&
	mkdir sub
'

test_expect_success 'backup-log add new item' '
	ID=$(git rev-parse HEAD:./initial.t) &&
	test_tick &&
	git -C sub backup-log --id=index update foo $ZERO_OID $ID &&
	echo "$ZERO_OID $ID $GIT_COMMITTER_NAME <$GIT_COMMITTER_EMAIL> $test_tick -0700	foo" >expected &&
	test_cmp expected .git/index.bkl
'

test_done
