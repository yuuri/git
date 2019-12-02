#!/bin/sh

test_description='add --pathspec-from-file'

. ./test-lib.sh

test_tick

test_expect_success setup '
	test_commit file0 &&
	echo A >fileA.t &&
	echo B >fileB.t &&
	echo C >fileC.t &&
	echo D >fileD.t
'

restore_checkpoint () {
	git reset
}

verify_expect () {
	git status --porcelain --untracked-files=no -- fileA.t fileB.t fileC.t fileD.t >actual &&
	test_cmp expect actual
}

test_expect_success 'simplest' '
	restore_checkpoint &&

	echo fileA.t | git add --pathspec-from-file=- &&

	cat >expect <<-\EOF &&
	A  fileA.t
	EOF
	verify_expect
'

test_expect_success 'only touches what was listed' '
	restore_checkpoint &&

	printf "fileB.t\nfileC.t\n" | git add --pathspec-from-file=- &&

	cat >expect <<-\EOF &&
	A  fileB.t
	A  fileC.t
	EOF
	verify_expect
'

test_done
