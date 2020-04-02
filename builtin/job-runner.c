#include "builtin.h"
#include "config.h"
#include "parse-options.h"
#include "run-command.h"
#include "string-list.h"

static char const * const builtin_job_runner_usage[] = {
	N_("git job-runner [<options>]"),
	NULL
};

static struct string_list arg_repos = STRING_LIST_INIT_DUP;

static int arg_repos_append(const struct option *opt,
			    const char *arg, int unset)
{
	string_list_append(&arg_repos, arg);
	return 0;
}

static int load_active_repos(struct string_list *repos)
{
	struct string_list_item *item;
	const struct string_list *config_repos;

	if (arg_repos.nr) {
		struct string_list_item *item;
		for (item = arg_repos.items;
		     item && item < arg_repos.items + arg_repos.nr;
		     item++)
			string_list_append(repos, item->string);
		return 0;
	}

	config_repos = git_config_get_value_multi("job.repo");

	for (item = config_repos->items;
	     item && item < config_repos->items + config_repos->nr;
	     item++) {
		DIR *dir = opendir(item->string);

		if (!dir)
			continue;

		closedir(dir);
		string_list_append(repos, item->string);
	}

	string_list_sort(repos);
	string_list_remove_duplicates(repos, 0);

	return 0;
}

static int run_job(const char *job, const char *repo)
{
	struct argv_array cmd = ARGV_ARRAY_INIT;
	argv_array_pushl(&cmd, "-C", repo, "run-job", job, NULL);
	return run_command_v_opt(cmd.argv, RUN_GIT_CMD);
}

static int run_job_loop_step(struct string_list *list)
{
	int result = 0;
	struct string_list_item *job;
	struct string_list repos = STRING_LIST_INIT_DUP;

	if ((result = load_active_repos(&repos)))
		return result;

	for (job = list->items;
	     !result && job && job < list->items + list->nr;
	     job++) {
		struct string_list_item *repo;
		for (repo = repos.items;
		     !result && repo && repo < repos.items + repos.nr;
		     repo++)
			result = run_job(job->string,
					 repo->string);
	}

	string_list_clear(&repos, 0);
	return result;
}

static unsigned int get_loop_interval(void)
{
	/* Default: 30 minutes */
	return 30 * 60;
}

static int initialize_jobs(struct string_list *list)
{
	string_list_append(list, "commit-graph");
	string_list_append(list, "fetch");
	string_list_append(list, "loose-objects");
	string_list_append(list, "pack-files");
	return 0;
}

int cmd_job_runner(int argc, const char **argv, const char *prefix)
{
	int result;
	struct string_list job_list = STRING_LIST_INIT_DUP;
	static struct option builtin_job_runner_options[] = {
		OPT_CALLBACK_F(0, "repo",
			       NULL,
			       N_("<path>"),
			       N_("run jobs on the repository at <path>"),
			       PARSE_OPT_NONEG, arg_repos_append),
		OPT_END(),
	};

	if (argc == 2 && !strcmp(argv[1], "-h"))
		usage_with_options(builtin_job_runner_usage,
				   builtin_job_runner_options);

	argc = parse_options(argc, argv, prefix,
			     builtin_job_runner_options,
			     builtin_job_runner_usage,
			     0);

	result = initialize_jobs(&job_list);

	while (!(result = run_job_loop_step(&job_list))) {
		unsigned int interval = get_loop_interval();
		sleep(interval);
	}

	return result;
}
