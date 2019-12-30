#!/bin/sh

test_description='reset --pathspec-from-file'

. ./test-lib.sh

test_tick

test_expect_success setup '
	echo A >fileA.t &&
	echo B >fileB.t &&
	echo C >fileC.t &&
	echo D >fileD.t &&
	git add . &&
	git commit --include . -m "Commit" &&
	git tag checkpoint
'

restore_checkpoint () {
	git reset --hard checkpoint
}

verify_expect () {
	git status --porcelain -- fileA.t fileB.t fileC.t fileD.t >actual &&
	test_cmp expect actual
}

test_expect_success 'simplest' '
	restore_checkpoint &&

	git rm fileA.t &&
	echo fileA.t | git reset --pathspec-from-file=- &&

	cat >expect <<-\EOF &&
	 D fileA.t
	EOF
	verify_expect
'

test_expect_success '--pathspec-file-nul' '
	restore_checkpoint &&

	git rm fileA.t fileB.t &&
	printf "fileA.t\0fileB.t\0" | git reset --pathspec-from-file=- --pathspec-file-nul &&

	cat >expect <<-\EOF &&
	 D fileA.t
	 D fileB.t
	EOF
	verify_expect
'

test_expect_success 'only touches what was listed' '
	restore_checkpoint &&

	git rm fileA.t fileB.t fileC.t fileD.t &&
	printf "fileB.t\nfileC.t\n" | git reset --pathspec-from-file=- &&

	cat >expect <<-\EOF &&
	D  fileA.t
	 D fileB.t
	 D fileC.t
	D  fileD.t
	EOF
	verify_expect
'

test_expect_success '--pathspec-from-file is not compatible with --soft or --hard' '
	restore_checkpoint &&

	git rm fileA.t &&
	echo fileA.t >list &&
	test_must_fail git reset --soft --pathspec-from-file=list &&
	test_must_fail git reset --hard --pathspec-from-file=list
'

test_expect_success 'only touches what was listed' '
	restore_checkpoint &&

	git rm fileA.t fileB.t fileC.t fileD.t &&
	printf "fileB.t\nfileC.t\n" | git reset --pathspec-from-file=- &&

	cat >expect <<-\EOF &&
	D  fileA.t
	 D fileB.t
	 D fileC.t
	D  fileD.t
	EOF
	verify_expect
'

test_done
