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

test_expect_success '--pathspec-file-nul' '
	restore_checkpoint &&

	printf "fileA.t\0fileB.t\0" | git add --pathspec-from-file=- --pathspec-file-nul &&

	cat >expect <<-\EOF &&
	A  fileA.t
	A  fileB.t
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

test_expect_success 'error conditions' '
	restore_checkpoint &&
	echo fileA.t >list &&
	>empty_list &&

	test_must_fail git add --pathspec-from-file=- --interactive <list 2>err &&
	test_i18ngrep "\-\-pathspec-from-file is incompatible with \-\-interactive/\-\-patch" err &&

	test_must_fail git add --pathspec-from-file=- --patch <list 2>err &&
	test_i18ngrep "\-\-pathspec-from-file is incompatible with \-\-interactive/\-\-patch" err &&

	test_must_fail git add --pathspec-from-file=- --edit <list 2>err &&
	test_i18ngrep "\-\-pathspec-from-file is incompatible with \-\-edit" err &&

	test_must_fail git add --pathspec-from-file=- -- fileA.t <list 2>err &&
	test_i18ngrep "\-\-pathspec-from-file is incompatible with pathspec arguments" err &&

	test_must_fail git add --pathspec-file-nul 2>err &&
	test_i18ngrep "\-\-pathspec-file-nul requires \-\-pathspec-from-file" err &&
	
	# This case succeeds, but still prints to stderr
	git add --pathspec-from-file=- <empty_list 2>err &&
	test_i18ngrep "Nothing specified, nothing added." err
'

test_done
