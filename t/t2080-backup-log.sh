#!/bin/sh

test_description='backup-log'

. ./test-lib.sh

test_expect_success 'setup' '
	test_commit initial &&
	mkdir sub
'

test_expect_success 'backup-log add new item' '
	ID=$(git rev-parse HEAD:./initial.t) &&
	test_tick &&
	git -C sub backup-log --id=index update foo $ZERO_OID $ID &&
	echo "$ZERO_OID $ID $GIT_COMMITTER_NAME <$GIT_COMMITTER_EMAIL> $test_tick -0700	foo" >expected &&
	test_cmp expected .git/index.bkl
'

test_expect_success 'add writes backup log' '
	test_tick &&
	echo update >>initial.t &&
	OLD=$(git rev-parse :./initial.t) &&
	NEW=$(git hash-object initial.t) &&
	git -c core.backupLog=true add initial.t &&
	echo "$OLD $NEW $GIT_COMMITTER_NAME <$GIT_COMMITTER_EMAIL> $test_tick -0700	initial.t" >expected &&
	tail -n1 .git/index.bkl >actual &&
	test_cmp expected actual
'

test_expect_success 'update-index --keep-backup writes backup log' '
	test_tick &&
	echo update-index >>initial.t &&
	OLD=$(git rev-parse :./initial.t) &&
	git -c core.backupLog=true update-index --keep-backup initial.t &&
	NEW=$(git hash-object initial.t) &&
	echo "$OLD $NEW $GIT_COMMITTER_NAME <$GIT_COMMITTER_EMAIL> $test_tick -0700	initial.t" >expected &&
	tail -n1 .git/index.bkl >actual &&
	test_cmp expected actual
'

test_expect_success 'commit -a writes backup log' '
	test_tick &&
	echo update-again >>initial.t &&
	OLD=$(git rev-parse :./initial.t) &&
	NEW=$(git hash-object initial.t) &&
	git -c core.backupLog=true commit -am update-again &&
	echo "$OLD $NEW $GIT_COMMITTER_NAME <$GIT_COMMITTER_EMAIL> $test_tick -0700	initial.t" >expected &&
	tail -n1 .git/index.bkl >actual &&
	test_cmp expected actual
'

test_expect_success 'partial commit writes backup log' '
	rm -f .git/index.bkl &&
	test_tick &&
	echo rising >>new-file &&
	git add -N new-file &&
	OLD=$EMPTY_BLOB &&
	NEW=$(git hash-object new-file) &&
	git -c core.backupLog=true commit -m update-once-more -- new-file initial.t &&
	echo "$OLD $NEW $GIT_COMMITTER_NAME <$GIT_COMMITTER_EMAIL> $test_tick -0700	new-file" >expected &&
	# expect file deletion to not log anything &&
	test_cmp expected .git/index.bkl
'

test_expect_success 'apply --cached writes backup log' '
	rm -f .git/index.bkl &&
	git reset --hard &&
	test_tick &&
	echo to-be-deleted >to-be-deleted &&
	echo to-be-modified >to-be-modified &&
	OLD_M=$(git hash-object to-be-modified) &&
	git add . &&
	git commit -m first &&
	rm to-be-deleted &&
	echo modified >>to-be-modified &&
	NEW_M=$(git hash-object to-be-modified) &&
	OLD_A=$ZERO_OID &&
	echo to-be-added >to-be-added &&
	NEW_A=$(git hash-object to-be-added) &&
	git add . &&
	git commit -m second &&
	git diff HEAD^ HEAD >patch &&
	git reset --hard HEAD^ &&
	git -c core.backupLog=true apply --cached --keep-backup patch &&
	echo "$OLD_A $NEW_A $GIT_COMMITTER_NAME <$GIT_COMMITTER_EMAIL> $test_tick -0700	to-be-added" >expected &&
	echo "$OLD_M $NEW_M $GIT_COMMITTER_NAME <$GIT_COMMITTER_EMAIL> $test_tick -0700	to-be-modified" >>expected &&
	test_cmp expected .git/index.bkl
'

test_expect_success 'backup-log cat' '
	OLD=$(git rev-parse :./initial.t) &&
	echo update >>initial.t &&
	test_tick &&
	git -c core.backupLog=true add initial.t &&
	NEW=$(git rev-parse :./initial.t) &&
	git backup-log --id=index cat --before --hash $test_tick initial.t >actual &&
	echo $OLD >expected &&
	test_cmp expected actual &&
	git backup-log --id=index cat --hash $test_tick initial.t >actual &&
	echo $NEW >expected &&
	test_cmp expected actual &&
	git backup-log --id=index cat $test_tick initial.t >actual &&
	git cat-file blob $NEW >expected &&
	test_cmp expected actual
'

test_done
