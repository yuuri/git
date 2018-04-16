#!/bin/sh

test_description='Test git show works'

. ./test-lib.sh

test_expect_success 'verify git show HEAD:foo works' '
	test_commit A &&
	git show HEAD:A.t >actual &&
	echo A >expected &&
	test_cmp expected actual
'

test_expect_success SYMLINKS 'verify git show HEAD:symlink shows symlink points to foo' '
	ln -s A.t A.link &&
	git add A.link &&
	git commit -m"Added symlink to A.t" &&
	git show HEAD:A.link >actual &&
	printf "%s" A.t >expected &&
	test_cmp expected actual
'

test_expect_success SYMLINKS 'verify git show --follow-symlinks HEAD:symlink shows foo' '
	git show --follow-symlinks HEAD:A.link >actual &&
	echo A >expected &&
	test_cmp expected actual
'

test_expect_success SYMLINKS 'verify git show --follow-symlinks HEAD:symlink works with subdirs' '
	mkdir dir &&
	ln -s dir symlink-to-dir &&
	test_commit dir/B &&
	git add dir symlink-to-dir &&
	git commit -m "add dir and symlink-to-dir" &&
	test_must_fail git show HEAD:symlink-to-dir/B.t >actual &&
	git show --follow-symlinks HEAD:symlink-to-dir/B.t >actual &&
	echo dir/B >expected &&
	test_cmp expected actual
'

test_done
