#include "cache.h"
#include "builtin.h"
#include "backup-log.h"
#include "parse-options.h"

static char const * const backup_log_usage[] = {
	N_("git backup-log [--path=<path> | --id=<id>] update <path> <old-hash> <new-hash>"),
	NULL
};

static int update(int argc, const char **argv,
		  const char *prefix, const char *log_path)
{
	struct strbuf sb = STRBUF_INIT;
	struct object_id oid_from, oid_to;
	const char *path;
	int ret;

	if (argc != 4)
		usage_with_options(backup_log_usage, NULL);

	path = argv[1];

	if (get_oid(argv[2], &oid_from))
		die(_("not a valid object name: %s"), argv[2]);

	if (get_oid(argv[3], &oid_to))
		die(_("not a valid object name: %s"), argv[3]);

	bkl_append(&sb, path, &oid_from, &oid_to);
	ret = bkl_write(log_path, &sb);
	strbuf_release(&sb);

	return ret;
}

static char *log_id_to_path(const char *id)
{
	if (!strcmp(id, "index"))
		return git_pathdup("index.bkl");
	else if (!strcmp(id, "worktree"))
		return git_pathdup("worktree.bkl");
	else if (!strcmp(id, "gitdir"))
		return git_pathdup("common/gitdir.bkl");
	else
		die(_("backup log id '%s' is not recognized"), id);
}

int cmd_backup_log(int argc, const char **argv, const char *prefix)
{
	const char *log_id = NULL;
	const char *log_path = NULL;
	char *to_free = NULL;
	struct option common_opts[] = {
		OPT_STRING(0, "id", &log_id, N_("id"), N_("backup log file id")),
		OPT_FILENAME(0, "path", &log_path, N_("backup log file path")),
		OPT_END()
	};

	argc = parse_options(argc, argv, prefix, common_opts,
			     backup_log_usage,
			     PARSE_OPT_STOP_AT_NON_OPTION);
	if (!argc)
		die(_("expected a subcommand"));

	if (log_id) {
		if (log_path)
			die(_("--id and --path are incompatible"));
		log_path = to_free = log_id_to_path(log_id);
	}
	if (!log_path)
		die(_("either --id or --path is required"));

	if (!strcmp(argv[0], "update"))
		return update(argc, argv, prefix, log_path);
	else
		die(_("unknown subcommand: %s"), argv[0]);

	free(to_free);
	return 0;
}
