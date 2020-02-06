#include "test-tool.h"
#include "cache.h"
#include "advice.h"

int cmd__advise_if_enabled(int argc, const char **argv)
{
	if (!argv[1])
	die("usage: %s <advice>", argv[0]);

	setup_git_directory();

	//use any advice type for testing
	advise_if_enabled(NESTED_TAG, argv[1]);

	return 0;
}
