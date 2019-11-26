#!/bin/sh

test_description='checkout --pathspec-from-file'

. ./test-lib.sh

test_tick

test_expect_success setup '
	test_commit file0 &&

	echo 1 >fileA.t &&
	echo 1 >fileB.t &&
	echo 1 >fileC.t &&
	echo 1 >fileD.t &&
	git add fileA.t fileB.t fileC.t fileD.t &&
	git commit -m "files 1" &&

	echo 2 >fileA.t &&
	echo 2 >fileB.t &&
	echo 2 >fileC.t &&
	echo 2 >fileD.t &&
	git add fileA.t fileB.t fileC.t fileD.t &&
	git commit -m "files 2" &&

	git tag checkpoint
'

restore_checkpoint () {
	git reset --hard checkpoint
}

verify_expect () {
	git status --porcelain --untracked-files=no -- fileA.t fileB.t fileC.t fileD.t >actual &&
	test_cmp expect actual
}

test_expect_success 'simplest' '
	restore_checkpoint &&

	echo fileA.t | git checkout --pathspec-from-file=- HEAD^1 &&

	cat >expect <<-\EOF &&
	M  fileA.t
	EOF
	verify_expect
'

test_expect_success '--pathspec-file-nul' '
	restore_checkpoint &&

	printf "fileA.t\0fileB.t\0" | git checkout --pathspec-from-file=- --pathspec-file-nul HEAD^1 &&

	cat >expect <<-\EOF &&
	M  fileA.t
	M  fileB.t
	EOF
	verify_expect
'

test_expect_success 'only touches what was listed' '
	restore_checkpoint &&

	printf "fileB.t\nfileC.t\n" | git checkout --pathspec-from-file=- HEAD^1 &&

	cat >expect <<-\EOF &&
	M  fileB.t
	M  fileC.t
	EOF
	verify_expect
'

test_expect_success 'error conditions' '
	restore_checkpoint &&
	echo fileA.t >list &&

	test_must_fail git checkout --pathspec-from-file=- --detach <list 2>err &&
	test_i18ngrep -e "--pathspec-from-file is incompatible with --detach" err &&

	test_must_fail git checkout --pathspec-from-file=- --patch <list 2>err &&
	test_i18ngrep -e "--pathspec-from-file is incompatible with --patch" err &&

	test_must_fail git checkout --pathspec-from-file=- -- fileA.t <list 2>err &&
	test_i18ngrep -e "--pathspec-from-file is incompatible with pathspec arguments" err &&

	test_must_fail git checkout --pathspec-file-nul 2>err &&
	test_i18ngrep -e "--pathspec-file-nul requires --pathspec-from-file" err
'

test_done
