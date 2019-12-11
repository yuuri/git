#!/bin/sh

test_description='Test parse_pathspec_file()'

. ./test-lib.sh

test_expect_success 'one item from stdin' '
	echo fileA.t | test-tool parse-pathspec-file --pathspec-from-file=- >actual &&

	cat >expect <<-\EOF &&
	fileA.t
	EOF
	test_cmp expect actual
'

test_expect_success 'one item from file' '
	echo fileA.t >list &&
	test-tool parse-pathspec-file --pathspec-from-file=list >actual &&

	cat >expect <<-\EOF &&
	fileA.t
	EOF
	test_cmp expect actual
'

test_expect_success 'NUL delimiters' '
	printf "fileA.t\0fileB.t\0" | test-tool parse-pathspec-file --pathspec-from-file=- --pathspec-file-nul >actual &&

	cat >expect <<-\EOF &&
	fileA.t
	fileB.t
	EOF
	test_cmp expect actual
'

test_expect_success 'LF delimiters' '
	printf "fileA.t\nfileB.t\n" | test-tool parse-pathspec-file --pathspec-from-file=- >actual &&

	cat >expect <<-\EOF &&
	fileA.t
	fileB.t
	EOF
	test_cmp expect actual
'

test_expect_success 'no trailing delimiter' '
	printf "fileA.t\nfileB.t" | test-tool parse-pathspec-file --pathspec-from-file=- >actual &&

	cat >expect <<-\EOF &&
	fileA.t
	fileB.t
	EOF
	test_cmp expect actual
'

test_expect_success 'CRLF delimiters' '
	printf "fileA.t\r\nfileB.t\r\n" | test-tool parse-pathspec-file --pathspec-from-file=- >actual &&

	cat >expect <<-\EOF &&
	fileA.t
	fileB.t
	EOF
	test_cmp expect actual
'

test_expect_success 'quotes' '
	# shell  takes \\\\101 and spits \\101
	# printf takes   \\101 and spits  \101
	# git    takes    \101 and spits     A
	printf "\"file\\\\101.t\"" | test-tool parse-pathspec-file --pathspec-from-file=- >actual &&

	cat >expect <<-\EOF &&
	fileA.t
	EOF
	test_cmp expect actual
'

test_expect_success '--pathspec-file-nul takes quotes literally' '
	# shell  takes \\\\101 and spits \\101
	# printf takes   \\101 and spits  \101
	printf "\"file\\\\101.t\"" | test-tool parse-pathspec-file --pathspec-from-file=- --pathspec-file-nul >actual &&

	cat >expect <<-\EOF &&
	"file\101.t"
	EOF
	test_cmp expect actual
'

test_done
