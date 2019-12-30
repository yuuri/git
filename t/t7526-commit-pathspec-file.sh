#!/bin/sh

test_description='commit --pathspec-from-file'

. ./test-lib.sh

test_tick

test_expect_success setup '
	test_commit file0 &&
	git tag checkpoint &&

	echo A >fileA.t &&
	echo B >fileB.t &&
	echo C >fileC.t &&
	echo D >fileD.t &&
	git add fileA.t fileB.t fileC.t fileD.t
'

restore_checkpoint () {
	git reset --soft checkpoint
}

verify_expect () {
	git diff-tree --no-commit-id --name-status -r HEAD >actual &&
	test_cmp expect actual
}

test_expect_success 'simplest' '
	restore_checkpoint &&

	echo fileA.t | git commit --pathspec-from-file=- -m "Commit" &&

	cat >expect <<-\EOF &&
	A	fileA.t
	EOF
	verify_expect
'

test_expect_success '--pathspec-file-nul' '
	restore_checkpoint &&

	printf "fileA.t\0fileB.t\0" | git commit --pathspec-from-file=- --pathspec-file-nul -m "Commit" &&

	cat >expect <<-\EOF &&
	A	fileA.t
	A	fileB.t
	EOF
	verify_expect
'

test_expect_success 'only touches what was listed' '
	restore_checkpoint &&

	printf "fileB.t\nfileC.t\n" | git commit --pathspec-from-file=- -m "Commit" &&

	cat >expect <<-\EOF &&
	A	fileB.t
	A	fileC.t
	EOF
	verify_expect
'

test_expect_success '--pathspec-from-file and --all cannot be used together' '
	restore_checkpoint &&
	test_must_fail git commit --pathspec-from-file=- --all -m "Commit" 2>err &&
	test_i18ngrep "[-]-pathspec-from-file with -a does not make sense" err
'

test_done
