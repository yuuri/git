#include "cache.h"
#include "builtin.h"
#include "backup-log.h"
#include "dir.h"
#include "object-store.h"
#include "parse-options.h"

static char const * const backup_log_usage[] = {
	N_("git backup-log [--path=<path> | --id=<id>] update <path> <old-hash> <new-hash>"),
	N_("git backup-log [--path=<path> | --id=<id>] cat [<options>] <change-id> <path>"),
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

struct cat_options {
	timestamp_t id;
	char *path;
	int before;
	int show_hash;
};

static int cat_parse(struct strbuf *line, void *data)
{
	struct cat_options *opts = data;
	struct bkl_entry entry;
	struct object_id *oid;
	void *buf;
	unsigned long size;
	enum object_type type;

	if (bkl_parse_entry(line, &entry))
		return -1;

	if (entry.timestamp < opts->id)
		return 2;

	if (entry.timestamp != opts->id ||
	    fspathcmp(entry.path, opts->path))
		return 0;

	if (opts->before)
		oid = &entry.old_oid;
	else
		oid = &entry.new_oid;

	if (opts->show_hash) {
		puts(oid_to_hex(oid));
		return 1;
	}

	if (is_null_oid(oid))
		return 1;	/* treat null oid like empty blob */

	buf = read_object_file(oid, &type, &size);
	if (!buf)
		die(_("object not found: %s"), oid_to_hex(oid));
	if (type != OBJ_BLOB)
		die(_("not a blob: %s"), oid_to_hex(oid));

	write_in_full(1, buf, size);
	free(buf);

	return 1;
}

static int cat(int argc, const char **argv,
		const char *prefix, const char *log_path)
{
	struct cat_options opts;
	struct option options[] = {
		OPT_BOOL(0, "before", &opts.before, N_("select the version before the update")),
		OPT_BOOL(0, "hash", &opts.show_hash, N_("show the hash instead of the content")),
		OPT_END()
	};
	char *end = NULL;
	int ret;

	memset(&opts, 0, sizeof(opts));
	argc = parse_options(argc, argv, prefix, options, backup_log_usage, 0);
	if (argc != 2)
		usage_with_options(backup_log_usage, options);
	opts.id = strtol(argv[0], &end, 10);
	if (!end || *end)
		die(_("not a valid change id: %s"), argv[0]);
	opts.path = prefix_path(prefix, prefix ? strlen(prefix) : 0, argv[1]);
	setup_pager();
	ret = bkl_parse_file_reverse(log_path, cat_parse, &opts);
	if (ret < 0)
		die(_("failed to parse '%s'"), log_path);
	return ret - 1;
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
	else if (!strcmp(argv[0], "cat"))
		return cat(argc, argv, prefix, log_path);
	else
		die(_("unknown subcommand: %s"), argv[0]);

	free(to_free);
	return 0;
}
