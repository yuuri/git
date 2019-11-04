#!/bin/sh

test_description='commit --pathspec-from-file'

. ./test-lib.sh

test_tick

cat > expect.a <<EOF
A	fileA.t
EOF

cat > expect.ab <<EOF
A	fileA.t
A	fileB.t
EOF

cat > expect.bc <<EOF
A	fileB.t
A	fileC.t
EOF

test_expect_success setup '
	test_commit file0 &&
	checkpoint=$(git rev-parse --verify HEAD) &&
	
	echo A >fileA.t &&
	echo B >fileB.t &&
	echo C >fileC.t &&
	echo D >fileD.t &&
	git add fileA.t fileB.t fileC.t fileD.t
'

restore_checkpoint () {
	git reset --soft "$checkpoint"
}

verify_state () {
	git diff-tree --no-commit-id --name-status -r HEAD >actual &&
	test_cmp "$1" actual
}

test_expect_success '--pathspec-from-file from stdin' '
	restore_checkpoint &&

	echo fileA.t | git commit --pathspec-from-file=- -m "Commit" &&

	verify_state expect.a
'

test_expect_success '--pathspec-from-file from file' '
	restore_checkpoint &&

	echo fileA.t >list &&
	git commit --pathspec-from-file=list -m "Commit" &&

	verify_state expect.a
'

test_expect_success 'NUL delimiters' '
	restore_checkpoint &&

	printf fileA.tQfileB.t | q_to_nul | git commit --pathspec-from-file=- --pathspec-file-null -m "Commit" &&

	verify_state expect.ab
'

test_expect_success 'LF delimiters' '
	restore_checkpoint &&

	printf "fileA.t\nfileB.t" | git commit --pathspec-from-file=- -m "Commit" &&

	verify_state expect.ab
'

test_expect_success 'CRLF delimiters' '
	restore_checkpoint &&

	printf "fileA.t\r\nfileB.t" | git commit --pathspec-from-file=- -m "Commit" &&

	verify_state expect.ab
'

test_expect_success 'quotes' '
	restore_checkpoint &&

	printf "\"file\\101.t\"" | git commit --pathspec-from-file=- -m "Commit" &&

	verify_state expect.a
'

test_expect_success 'quotes not compatible with --pathspec-file-null' '
	restore_checkpoint &&

	printf "\"file\\101.t\"" >list &&
	test_must_fail git commit --pathspec-from-file=list --pathspec-file-null -m "Commit"
'

test_expect_success 'only touches what was listed' '
	restore_checkpoint &&

	printf "fileB.t\nfileC.t" | git commit --pathspec-from-file=- -m "Commit" &&

	verify_state expect.bc
'

test_done
