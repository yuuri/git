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

test_expect_success 'fetch job' '
	git clone "file://$(pwd)/server" client &&

	# Before fetching, build a client commit-graph
	git -C client run-job commit-graph &&
	chain=client/.git/objects/info/commit-graphs/commit-graph-chain &&
	test_line_count = 1 $chain &&

	git -C client branch -v --remotes >before-refs &&
	test_commit -C server 24 &&

	git -C client run-job fetch &&
	git -C client branch -v --remotes >after-refs &&
	test_cmp before-refs after-refs &&
	test_cmp server/.git/refs/heads/master \
		 client/.git/refs/hidden/origin/master &&

	# the hidden ref should trigger a new layer in the commit-graph
	git -C client run-job commit-graph &&
	test_line_count = 2 $chain
'

test_done
