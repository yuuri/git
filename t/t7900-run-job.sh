#!/bin/sh

test_description='git run-job

Testing the background jobs, in the foreground
'

. ./test-lib.sh

test_expect_success 'help text' '
	test_must_fail git run-job -h 2>err &&
	test_i18ngrep "usage: git run-job" err
'

test_done
