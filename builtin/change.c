#include "builtin.h"
#include "ref-filter.h"
#include "parse-options.h"
#include "metacommit.h"
#include "config.h"

static const char * const builtin_change_usage[] = {
	N_("git change list [<pattern>...]"),
	N_("git change update [--force] [--replace <treeish>...] [--origin <treesih>...] [--content <newtreeish>]"),
	NULL
};

static const char * const builtin_list_usage[] = {
	N_("git change list [<pattern>...]"),
	NULL
};

static const char * const builtin_update_usage[] = {
	N_("git change update [--force] [--replace <treeish>...] [--origin <treesih>...] [--content <newtreeish>]"),
	NULL
};

static int change_list(int argc, const char **argv, const char* prefix)
{
	struct option options[] = {
		OPT_END()
	};
	struct ref_filter filter;
	// TODO: Sorting temporarily disabled. See comments, below.
	//struct ref_sorting *sorting = ref_default_sorting();
	struct ref_format format = REF_FORMAT_INIT;
	struct ref_array array;
	int i;

	argc = parse_options(argc, argv, prefix, options, builtin_list_usage, 0);

	setup_ref_filter_porcelain_msg();

	memset(&filter, 0, sizeof(filter));
	memset(&array, 0, sizeof(array));

	filter.kind = FILTER_REFS_CHANGES;
	filter.name_patterns = argv;

	filter_refs(&array, &filter, FILTER_REFS_CHANGES);

	// TODO: This causes a crash. It sets one of the atom_value handlers to
	// something invalid, which causes a crash later when we call
	// show_ref_array_item. Figure out why this happens and put back the sorting.
	//ref_array_sort(sorting, &array);

	if (!format.format) {
		format.format = "%(refname:lstrip=1)";
	}

	if (verify_ref_format(&format))
		die(_("unable to parse format string"));

	for (i = 0; i < array.nr; i++) {
		show_ref_array_item(array.items[i], &format);
	}

	ref_array_clear(&array);

	return 0;
}

struct update_state {
	int options;
	const char* change;
	const char* content;
	struct string_list replace;
	struct string_list origin;
};

static void init_update_state(struct update_state *state)
{
	memset(state, 0, sizeof(*state));
	state->content = "HEAD";
	string_list_init(&state->replace, 0);
	string_list_init(&state->origin, 0);
}

static void clear_update_state(struct update_state *state)
{
	string_list_clear(&state->replace, 0);
	string_list_clear(&state->origin, 0);
}

static int update_option_parse_replace(const struct option *opt,
							const char *arg, int unset)
{
	struct update_state *state = opt->value;
	string_list_append(&state->replace, arg);
	return 0;
}

static int update_option_parse_origin(const struct option *opt,
							const char *arg, int unset)
{
	struct update_state *state = opt->value;
	string_list_append(&state->origin, arg);
	return 0;
}

static int resolve_commit(const char *committish, struct object_id *result)
{
	struct commit *commit;
	if (get_oid_committish(committish, result))
		die(_("Failed to resolve '%s' as a valid revision."), committish);
	commit = lookup_commit_reference(the_repository, result);
	if (!commit)
		die(_("Could not parse object '%s'."), committish);
	oidcpy(result, &commit->object.oid);
	return 0;
}

static void resolve_commit_list(const struct string_list *commitsish_list,
	struct oid_array* result)
{
	int i;
	for (i = 0; i < commitsish_list->nr; i++) {
		struct string_list_item *item = &commitsish_list->items[i];
		struct object_id next;
		resolve_commit(item->string, &next);
		oid_array_append(result, &next);
	}
}

/*
 * Given the command-line options for the update command, fills in a
 * metacommit_data with the corresponding changes.
 */
static void get_metacommit_from_command_line(
	const struct update_state* commands, struct metacommit_data *result)
{
	resolve_commit(commands->content, &(result->content));
	resolve_commit_list(&(commands->replace), &(result->replace));
	resolve_commit_list(&(commands->origin), &(result->origin));
}

static int perform_update(
	struct repository *repo,
	const struct update_state *state,
	struct strbuf *err)
{
	struct metacommit_data metacommit;
	int ret;

	init_metacommit_data(&metacommit);

	get_metacommit_from_command_line(state, &metacommit);

	ret = record_metacommit(repo, &metacommit, state->change, state->options, err);

	clear_metacommit_data(&metacommit);

	return ret;
}

static int change_update(int argc, const char **argv, const char* prefix)
{
	int result;
	int force = 0;
	int newchange = 0;
	struct strbuf err = STRBUF_INIT;
	struct update_state state;
	struct option options[] = {
		{ OPTION_CALLBACK, 'r', "replace", &state, N_("commit"),
			N_("marks the given commit as being obsolete"),
			0, update_option_parse_replace },
		{ OPTION_CALLBACK, 'o', "origin", &state, N_("commit"),
			N_("marks the given commit as being the origin of this commit"),
			0, update_option_parse_origin },
		OPT_BOOL('F', "force", &force,
			N_("overwrite an existing change of the same name")),
		OPT_STRING('c', "content", &state.content, N_("commit"),
				 N_("identifies the new content commit for the change")),
		OPT_STRING('g', "change", &state.change, N_("commit"),
				 N_("name of the change to update")),
		OPT_BOOL('n', "new", &newchange,
			N_("create a new change - do not append to any existing change")),
		OPT_END()
	};

	init_update_state(&state);

	argc = parse_options(argc, argv, prefix, options, builtin_update_usage, 0);

	if (force) state.options |= UPDATE_OPTION_FORCE;
	if (newchange) state.options |= UPDATE_OPTION_NOAPPEND;

	result = perform_update(the_repository, &state, &err);

	if (result < 0) {
		error("%s", err.buf);
		strbuf_release(&err);
	}

	clear_update_state(&state);

	return result;
}

int cmd_change(int argc, const char **argv, const char *prefix)
{
	// No options permitted before subcommand currently
	struct option options[] = {
		OPT_END()
	};
	int result = 1;

	argc = parse_options(argc, argv, prefix, options, builtin_change_usage,
		PARSE_OPT_STOP_AT_NON_OPTION);

	if (argc < 1)
		usage_with_options(builtin_change_usage, options);
	else if (!strcmp(argv[0], "list"))
		result = change_list(argc, argv, prefix);
	else if (!strcmp(argv[0], "update"))
		result = change_update(argc, argv, prefix);
	else {
		error(_("Unknown subcommand: %s"), argv[0]);
		usage_with_options(builtin_change_usage, options);
	}

	return result ? 1 : 0;
}
