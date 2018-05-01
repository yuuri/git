#include "cache.h"
#include "builtin.h"
#include "parse-options.h"

static const char * const builtin_branch_diff_usage[] = {
N_("git branch-diff [<options>] <old-base>..<old-tip> <new-base>..<new-tip>"),
N_("git branch-diff [<options>] <old-tip>...<new-tip>"),
N_("git branch-diff [<options>] <base> <old-tip> <new-tip>"),
NULL
};

static int parse_creation_weight(const struct option *opt, const char *arg,
				 int unset)
{
	double *d = opt->value;
	if (unset)
		*d = 0.6;
	else
		*d = atof(arg);
	return 0;
}

int cmd_branch_diff(int argc, const char **argv, const char *prefix)
{
	double creation_weight = 0.6;
	struct option options[] = {
		{ OPTION_CALLBACK,
			0, "creation-weight", &creation_weight, N_("factor"),
			N_("Fudge factor by which creation is weighted [0.6]"),
			0, parse_creation_weight },
		OPT_END()
	};

	argc = parse_options(argc, argv, NULL, options,
			builtin_branch_diff_usage, 0);

	return 0;
}
