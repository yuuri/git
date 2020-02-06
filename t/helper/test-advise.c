#include "test-tool.h"
#include "cache.h"
#include "advice.h"

int cmd__advise_ng(int argc, const char **argv)
{
	if (!argv[1] || !argv[2])
	die("usage: %s <key> <advice>", argv[0]);

	setup_git_directory();

	advise_ng(argv[1], argv[2]);

	return 0;
}
