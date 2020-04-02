#include "builtin.h"
#include "config.h"
#include "parse-options.h"

static char const * const builtin_run_job_usage[] = {
	N_("git run-job"),
	NULL
};

int cmd_run_job(int argc, const char **argv, const char *prefix)
{
	static struct option builtin_run_job_options[] = {
		OPT_END(),
	};

	if (argc == 2 && !strcmp(argv[1], "-h"))
		usage_with_options(builtin_run_job_usage,
				   builtin_run_job_options);

	git_config(git_default_config, NULL);
	argc = parse_options(argc, argv, prefix,
			     builtin_run_job_options,
			     builtin_run_job_usage,
			     PARSE_OPT_KEEP_UNKNOWN);

	usage_with_options(builtin_run_job_usage,
			   builtin_run_job_options);
}
