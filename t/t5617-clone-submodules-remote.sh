#!/bin/sh

test_description='Test cloning repos with submodules using remote-tracking branches'

. ./test-lib.sh

pwd=$(pwd)

test_expect_success 'setup' '
	git checkout -b master &&
	test_commit commit1 &&
	mkdir sub &&
	(
		cd sub &&
		git init &&
		test_commit subcommit1 &&
		git tag sub_when_added_to_super
	) &&
	git submodule add "file://$pwd/sub" sub &&
	git commit -m "add submodule" &&
	(
		cd sub &&
		test_commit subcommit2
	)
'

# bare clone giving "srv.bare" for use as our server.
test_expect_success 'setup bare clone for server' '
	git clone --bare "file://$(pwd)/." srv.bare &&
	git -C srv.bare config --local uploadpack.allowfilter 1 &&
	git -C srv.bare config --local uploadpack.allowanysha1inwant 1
'

test_expect_success 'clone with --no-remote-submodules' '
	test_when_finished "rm -rf super_clone" &&
	git clone --recurse-submodules --no-remote-submodules "file://$pwd/." super_clone &&
	(
		cd super_clone/sub &&
		git diff --exit-code sub_when_added_to_super
	)
'

test_expect_success 'clone with --remote-submodules' '
	test_when_finished "rm -rf super_clone" &&
	git clone --recurse-submodules --remote-submodules "file://$pwd/." super_clone &&
	(
		cd super_clone/sub &&
		git diff --exit-code remotes/origin/master
	)
'

test_expect_success 'check the default is --no-remote-submodules' '
	test_when_finished "rm -rf super_clone" &&
	git clone --recurse-submodules "file://$pwd/." super_clone &&
	(
		cd super_clone/sub &&
		git diff --exit-code sub_when_added_to_super
	)
'

# do basic partial clone from "srv.bare"
# confirm partial clone was registered in the local config for super and sub.
test_expect_success 'clone with --filter' '
	git clone --recurse-submodules --filter blob:none "file://$pwd/srv.bare" super_clone &&
	test "$(git -C super_clone config --local core.repositoryformatversion)" = "1" &&
	test "$(git -C super_clone config --local extensions.partialclone)" = "origin" &&
	test "$(git -C super_clone config --local core.partialclonefilter)" = "blob:none" &&
	test "$(git -C super_clone/sub config --local core.repositoryformatversion)" = "1" &&
	test "$(git -C super_clone/sub config --local extensions.partialclone)" = "origin" &&
	test "$(git -C super_clone/sub config --local core.partialclonefilter)" = "blob:none"
'

test_done
