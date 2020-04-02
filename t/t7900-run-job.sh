#!/bin/sh

test_description='git run-job

Testing the background jobs, in the foreground
'

GIT_TEST_COMMIT_GRAPH=0

. ./test-lib.sh

test_expect_success 'help text' '
	test_must_fail git run-job -h 2>err &&
	test_i18ngrep "usage: git run-job" err
'

test_expect_success 'commit-graph job' '
	git init server &&
	(
		cd server &&
		chain=.git/objects/info/commit-graphs/commit-graph-chain &&
		git checkout -b master &&
		for i in $(test_seq 1 15)
		do
			test_commit $i || return 1
		done &&
		git run-job commit-graph 2>../err &&
		test_must_be_empty ../err &&
		test_line_count = 1 $chain &&
		for i in $(test_seq 16 19)
		do
			test_commit $i || return 1
		done &&
		git run-job commit-graph 2>../err &&
		test_must_be_empty ../err &&
		test_line_count = 2 $chain &&
		for i in $(test_seq 20 23)
		do
			test_commit $i || return 1
		done &&
		git run-job commit-graph 2>../err &&
		test_must_be_empty ../err &&
		test_line_count = 1 $chain
	)
'

test_done
