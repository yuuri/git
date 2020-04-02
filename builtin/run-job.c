#include "builtin.h"
#include "config.h"
#include "commit-graph.h"
#include "object-store.h"
#include "parse-options.h"
#include "repository.h"
#include "run-command.h"

static char const * const builtin_run_job_usage[] = {
	N_("git run-job (commit-graph|fetch)"),
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

static int fetch_remote(const char *remote)
{
	int result;
	struct argv_array cmd = ARGV_ARRAY_INIT;
	struct strbuf refmap = STRBUF_INIT;

	argv_array_pushl(&cmd, "fetch", remote, "--quiet", "--prune",
			 "--no-tags", "--refmap=", NULL);

	strbuf_addf(&refmap, "+refs/heads/*:refs/hidden/%s/*", remote);
	argv_array_push(&cmd, refmap.buf);

	result = run_command_v_opt(cmd.argv, RUN_GIT_CMD);

	strbuf_release(&refmap);
	return result;
}

static int fill_remotes(struct string_list *remotes)
{
	int result = 0;
	FILE *proc_out;
	struct strbuf line = STRBUF_INIT;
	struct child_process *remote_proc = xmalloc(sizeof(*remote_proc));

	child_process_init(remote_proc);

	argv_array_pushl(&remote_proc->args, "git", "remote", NULL);

	remote_proc->out = -1;

	if (start_command(remote_proc)) {
		error(_("failed to start 'git remote' process"));
		result = 1;
		goto cleanup;
	}

	proc_out = xfdopen(remote_proc->out, "r");

	/* if there is no line, leave the value as given */
	while (!strbuf_getline(&line, proc_out))
		string_list_append(remotes, line.buf);

	strbuf_release(&line);

	fclose(proc_out);

	if (finish_command(remote_proc)) {
		error(_("failed to finish 'git remote' process"));
		result = 1;
	}

cleanup:
	free(remote_proc);
	return result;
}

static int run_fetch_job(void)
{
	int result = 0;
	struct string_list_item *item;
	struct string_list remotes = STRING_LIST_INIT_DUP;

	if (fill_remotes(&remotes)) {
		error(_("failed to fill remotes"));
		result = 1;
		goto cleanup;
	}

	/*
	 * Do not modify the result based on the success of the 'fetch'
	 * operation, as a loss of network could cause 'fetch' to fail
	 * quickly. We do not want that to stop the rest of our
	 * background operations.
	 */
	for (item = remotes.items;
	     item && item < remotes.items + remotes.nr;
	     item++)
		fetch_remote(item->string);

cleanup:
	string_list_clear(&remotes, 0);
	return result;
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
		if (!strcmp(argv[0], "fetch"))
			return run_fetch_job();
	}

	usage_with_options(builtin_run_job_usage,
			   builtin_run_job_options);
}
