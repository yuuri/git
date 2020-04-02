#include "builtin.h"
#include "config.h"
#include "commit-graph.h"
#include "object-store.h"
#include "parse-options.h"
#include "repository.h"
#include "run-command.h"

static char const * const builtin_run_job_usage[] = {
	N_("git run-job commit-graph"),
	NULL
};

static int run_write_commit_graph(void)
{
	struct argv_array cmd = ARGV_ARRAY_INIT;

	argv_array_pushl(&cmd, "commit-graph", "write",
			 "--split", "--reachable", "--no-progress",
			 NULL);

	return run_command_v_opt(cmd.argv, RUN_GIT_CMD);
}

static int run_verify_commit_graph(void)
{
	struct argv_array cmd = ARGV_ARRAY_INIT;

	argv_array_pushl(&cmd, "commit-graph", "verify",
			 "--shallow", "--no-progress", NULL);

	return run_command_v_opt(cmd.argv, RUN_GIT_CMD);
}

static int run_commit_graph_job(void)
{
	char *chain_path;

	if (run_write_commit_graph()) {
		error(_("failed to write commit-graph"));
		return 1;
	}

	if (!run_verify_commit_graph())
		return 0;

	warning(_("commit-graph verify caught error, rewriting"));

	chain_path = get_chain_filename(the_repository->objects->odb);
	if (unlink(chain_path)) {
		UNLEAK(chain_path);
		die(_("failed to remove commit-graph at %s"), chain_path);
	}
	free(chain_path);

	if (!run_write_commit_graph())
		return 0;

	error(_("failed to rewrite commit-graph"));
	return 1;
}

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

	if (argc > 0) {
		if (!strcmp(argv[0], "commit-graph"))
			return run_commit_graph_job();
	}

	usage_with_options(builtin_run_job_usage,
			   builtin_run_job_options);
}
