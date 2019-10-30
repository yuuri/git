#!/bin/sh

test_description='reset --pathspec-from-file'

. ./test-lib.sh

cat > expect.a <<EOF
 D fileA.t
EOF

cat > expect.ab <<EOF
 D fileA.t
 D fileB.t
EOF

cat > expect.a_bc_d <<EOF
D  fileA.t
 D fileB.t
 D fileC.t
D  fileD.t
EOF

test_expect_success setup '
	echo A >fileA.t &&
	echo B >fileB.t &&
	echo C >fileC.t &&
	echo D >fileD.t &&
	git add . &&
	git commit --include . -m "Commit" &&
	checkpoint=$(git rev-parse --verify HEAD)
'

restore_checkpoint () {
	git reset --hard "$checkpoint"
}

verify_state () {
	git status --porcelain -- fileA.t fileB.t fileC.t fileD.t >actual &&
	test_cmp "$1" actual
}

test_expect_success '--pathspec-from-file from stdin' '
	restore_checkpoint &&

	git rm fileA.t &&
	echo fileA.t | git reset --pathspec-from-file=- &&

	verify_state expect.a
'

test_expect_success '--pathspec-from-file from file' '
	restore_checkpoint &&

	git rm fileA.t &&
	echo fileA.t >list &&
	git reset --pathspec-from-file=list &&

	verify_state expect.a
'

test_expect_success 'NUL delimiters' '
	restore_checkpoint &&

	git rm fileA.t fileB.t &&
	printf fileA.tQfileB.t | q_to_nul | git reset --pathspec-from-file=- --pathspec-file-null &&

	verify_state expect.ab
'

test_expect_success 'LF delimiters' '
	restore_checkpoint &&

	git rm fileA.t fileB.t &&
	printf "fileA.t\nfileB.t" | git reset --pathspec-from-file=- &&

	verify_state expect.ab
'

test_expect_success 'CRLF delimiters' '
	restore_checkpoint &&

	git rm fileA.t fileB.t &&
	printf "fileA.t\r\nfileB.t" | git reset --pathspec-from-file=- &&

	verify_state expect.ab
'

test_expect_success 'quotes' '
	restore_checkpoint &&

	git rm fileA.t &&
	printf "\"file\\101.t\"" | git reset --pathspec-from-file=- &&

	verify_state expect.a
'

test_expect_success 'quotes not compatible with --pathspec-file-null' '
	restore_checkpoint &&

	git rm fileA.t &&
	printf "\"file\\101.t\"" >list &&
	# Note: "git reset" has not yet learned to fail on wrong pathspecs
	git reset --pathspec-from-file=list --pathspec-file-null &&
	
	test_must_fail verify_state expect.a
'

test_expect_success '--pathspec-from-file is not compatible with --soft --hard' '
	restore_checkpoint &&

	git rm fileA.t &&
	echo fileA.t >list &&
	test_must_fail git reset --soft --pathspec-from-file=list &&
	test_must_fail git reset --hard --pathspec-from-file=list
'

test_expect_success 'only touches what was listed' '
	restore_checkpoint &&

	git rm fileA.t fileB.t fileC.t fileD.t &&
	printf "fileB.t\nfileC.t" | git reset --pathspec-from-file=- &&

	verify_state expect.a_bc_d
'

test_done
