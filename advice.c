#include "cache.h"
#include "config.h"
#include "color.h"
#include "help.h"

int advice_fetch_show_forced_updates = 1;
int advice_push_update_rejected = 1;
int advice_push_non_ff_current = 1;
int advice_push_non_ff_matching = 1;
int advice_push_already_exists = 1;
int advice_push_fetch_first = 1;
int advice_push_needs_force = 1;
int advice_push_unqualified_ref_name = 1;
int advice_status_hints = 1;
int advice_status_u_option = 1;
int advice_status_ahead_behind_warning = 1;
int advice_commit_before_merge = 1;
int advice_reset_quiet_warning = 1;
int advice_resolve_conflict = 1;
int advice_sequencer_in_use = 1;
int advice_implicit_identity = 1;
int advice_detached_head = 1;
int advice_set_upstream_failure = 1;
int advice_object_name_warning = 1;
int advice_amworkdir = 1;
int advice_rm_hints = 1;
int advice_add_embedded_repo = 1;
int advice_ignored_hook = 1;
int advice_waiting_for_editor = 1;
int advice_graft_file_deprecated = 1;
int advice_checkout_ambiguous_remote_branch_name = 1;
int advice_submodule_alternate_error_strategy_die = 1;

const char ADVICE_ADD_EMBEDDED_REPO[] = "advice.addEmbeddedRepo";
const char ADVICE_AM_WORK_DIR[] = "advice.amWorkDir";
const char ADVICE_CHECKOUT_AMBIGUOUS_REMOTE_BRANCH_NAME[] = "advice.checkoutAmbiguousRemoteBranchName";
const char ADVICE_COMMIT_BEFORE_MERGE[] = "advice.commitBeforeMerge";
const char ADVICE_DETACHED_HEAD[] = "advice.detachedHead";
const char ADVICE_FETCH_SHOW_FORCED_UPDATES[] = "advice.fetchShowForcedUpdates";
const char ADVICE_GRAFT_FILE_DEPRECATED[] = "advice.graftFileDeprecated";
const char ADVICE_IGNORED_HOOK[] = "advice.ignoredHook";
const char ADVICE_IMPLICIT_IDENTITY[] = "advice.implicitIdentity";
const char ADVICE_NESTED_TAG[] = "advice.nestedTag";
const char ADVICE_OBJECT_NAME_WARNING[] = "advice.objectNameWarning";
const char ADVICE_PUSH_ALREADY_EXISTS[] = "advice.pushAlreadyExists";
const char ADVICE_PUSH_FETCH_FIRST[] = "advice.pushFetchFirst";
const char ADVICE_PUSH_NEEDS_FORCE[] = "advice.pushNeedsForce";
const char ADVICE_PUSH_NON_FF_CURRENT[] = "advice.pushNonFFCurrent";
const char ADVICE_PUSH_NON_FF_MATCHING[] = "advice.pushNonFFMatching";
const char ADVICE_PUSH_UNQUALIFIED_REF_NAME[] = "advice.pushUnqualifiedRefName";
const char ADVICE_PUSH_UPDATE_REJECTED[] = "advice.pushUpdateRejected";

/* make this an alias for backward compatibility */
const char ADVICE_PUSH_UPDATE_REJECTED_ALIAS[] = "advice.pushNonFastForward";

const char ADVICE_RESET_QUIET_WARNING[] = "advice.resetQuiet";
const char ADVICE_RESOLVE_CONFLICT[] = "advice.resolveConflict";
const char ADVICE_RM_HINTS[] = "advice.rmHints";
const char ADVICE_SEQUENCER_IN_USE[] = "advice.sequencerInUse";
const char ADVICE_SET_UPSTREAM_FAILURE[] = "advice.setUpstreamFailure";
const char ADVICE_STATUS_AHEAD_BEHIND_WARNING[] = "advice.statusAheadBehindWarning";
const char ADVICE_STATUS_HINTS[] = "advice.statusHints";
const char ADVICE_STATUS_U_OPTION[] = "advice.statusUoption";
const char ADVICE_SUBMODULE_ALTERNATE_ERROR_STRATEGY_DIE[] = "advice.submoduleAlternateErrorStrategyDie";
const char ADVICE_WAITING_FOR_EDITOR[] = "advice.waitingForEditor";


static int advice_use_color = -1;
static char advice_colors[][COLOR_MAXLEN] = {
	GIT_COLOR_RESET,
	GIT_COLOR_YELLOW,	/* HINT */
};

enum color_advice {
	ADVICE_COLOR_RESET = 0,
	ADVICE_COLOR_HINT = 1,
};

static int parse_advise_color_slot(const char *slot)
{
	if (!strcasecmp(slot, "reset"))
		return ADVICE_COLOR_RESET;
	if (!strcasecmp(slot, "hint"))
		return ADVICE_COLOR_HINT;
	return -1;
}

static const char *advise_get_color(enum color_advice ix)
{
	if (want_color_stderr(advice_use_color))
		return advice_colors[ix];
	return "";
}

static struct {
	const char *name;
	int *preference;
} advice_config[] = {
	{ "fetchShowForcedUpdates", &advice_fetch_show_forced_updates },
	{ "pushUpdateRejected", &advice_push_update_rejected },
	{ "pushNonFFCurrent", &advice_push_non_ff_current },
	{ "pushNonFFMatching", &advice_push_non_ff_matching },
	{ "pushAlreadyExists", &advice_push_already_exists },
	{ "pushFetchFirst", &advice_push_fetch_first },
	{ "pushNeedsForce", &advice_push_needs_force },
	{ "pushUnqualifiedRefName", &advice_push_unqualified_ref_name },
	{ "statusHints", &advice_status_hints },
	{ "statusUoption", &advice_status_u_option },
	{ "statusAheadBehindWarning", &advice_status_ahead_behind_warning },
	{ "commitBeforeMerge", &advice_commit_before_merge },
	{ "resetQuiet", &advice_reset_quiet_warning },
	{ "resolveConflict", &advice_resolve_conflict },
	{ "sequencerInUse", &advice_sequencer_in_use },
	{ "implicitIdentity", &advice_implicit_identity },
	{ "detachedHead", &advice_detached_head },
	{ "setUpstreamFailure", &advice_set_upstream_failure },
	{ "objectNameWarning", &advice_object_name_warning },
	{ "amWorkDir", &advice_amworkdir },
	{ "rmHints", &advice_rm_hints },
	{ "addEmbeddedRepo", &advice_add_embedded_repo },
	{ "ignoredHook", &advice_ignored_hook },
	{ "waitingForEditor", &advice_waiting_for_editor },
	{ "graftFileDeprecated", &advice_graft_file_deprecated },
	{ "checkoutAmbiguousRemoteBranchName", &advice_checkout_ambiguous_remote_branch_name },
	{ "submoduleAlternateErrorStrategyDie", &advice_submodule_alternate_error_strategy_die },

	/* make this an alias for backward compatibility */
	{ "pushNonFastForward", &advice_push_update_rejected }
};

static const char *advice_config_keys[] = {
	ADVICE_ADD_EMBEDDED_REPO,
	ADVICE_AM_WORK_DIR,
	ADVICE_CHECKOUT_AMBIGUOUS_REMOTE_BRANCH_NAME,
	ADVICE_COMMIT_BEFORE_MERGE,
	ADVICE_DETACHED_HEAD,
	ADVICE_FETCH_SHOW_FORCED_UPDATES,
	ADVICE_GRAFT_FILE_DEPRECATED,
	ADVICE_IGNORED_HOOK,
	ADVICE_IMPLICIT_IDENTITY,
	ADVICE_NESTED_TAG,
	ADVICE_OBJECT_NAME_WARNING,
	ADVICE_PUSH_ALREADY_EXISTS,
	ADVICE_PUSH_FETCH_FIRST,
	ADVICE_PUSH_NEEDS_FORCE,
	ADVICE_PUSH_UPDATE_REJECTED_ALIAS,
	ADVICE_PUSH_NON_FF_CURRENT,
	ADVICE_PUSH_NON_FF_MATCHING,
	ADVICE_PUSH_UNQUALIFIED_REF_NAME,
	ADVICE_PUSH_UPDATE_REJECTED,
	ADVICE_RESET_QUIET_WARNING,
	ADVICE_RESOLVE_CONFLICT,
	ADVICE_RM_HINTS,
	ADVICE_SEQUENCER_IN_USE,
	ADVICE_SET_UPSTREAM_FAILURE,
	ADVICE_STATUS_AHEAD_BEHIND_WARNING,
	ADVICE_STATUS_HINTS,
	ADVICE_STATUS_U_OPTION,
	ADVICE_SUBMODULE_ALTERNATE_ERROR_STRATEGY_DIE,
	ADVICE_WAITING_FOR_EDITOR,
};

static const char turn_off_instructions[] =
N_("\n"
   "Disable this message with \"git config %s false\"");

static void vadvise(const char *advice, int display_instructions,
		    const char *key, va_list params)
{
	struct strbuf buf = STRBUF_INIT;
	const char *cp, *np;

	strbuf_vaddf(&buf, advice, params);

	if (display_instructions)
		strbuf_addf(&buf, turn_off_instructions, key);

	for (cp = buf.buf; *cp; cp = np) {
		np = strchrnul(cp, '\n');
		fprintf(stderr,	_("%shint: %.*s%s\n"),
			advise_get_color(ADVICE_COLOR_HINT),
			(int)(np - cp), cp,
			advise_get_color(ADVICE_COLOR_RESET));
		if (*np)
			np++;
	}
	strbuf_release(&buf);
}

void advise(const char *advice, ...)
{
	va_list params;
	va_start(params, advice);
	vadvise(advice, 0, "", params);
	va_end(params);
}

static int get_config_value(const char *advice_key)
{
	int value = 1;

	git_config_get_bool(advice_key, &value);
	return value;
}

int advice_enabled(const char *advice_key)
{
	if (advice_key == ADVICE_PUSH_UPDATE_REJECTED)
		return get_config_value(ADVICE_PUSH_UPDATE_REJECTED) &&
		       get_config_value(ADVICE_PUSH_UPDATE_REJECTED_ALIAS);
	else
		return get_config_value(advice_key);
}

void advise_if_enabled(const char *advice_key, const char *advice, ...)
{
	va_list params;

	if (!advice_enabled(advice_key))
		return;

	va_start(params, advice);
	vadvise(advice, 1, advice_key, params);
	va_end(params);
}

int git_default_advice_config(const char *var, const char *value)
{
	const char *k, *slot_name;
	int i;

	if (!strcmp(var, "color.advice")) {
		advice_use_color = git_config_colorbool(var, value);
		return 0;
	}

	if (skip_prefix(var, "color.advice.", &slot_name)) {
		int slot = parse_advise_color_slot(slot_name);
		if (slot < 0)
			return 0;
		if (!value)
			return config_error_nonbool(var);
		return color_parse(value, advice_colors[slot]);
	}

	if (!skip_prefix(var, "advice.", &k))
		return 0;

	for (i = 0; i < ARRAY_SIZE(advice_config); i++) {
		if (strcasecmp(k, advice_config[i].name))
			continue;
		*advice_config[i].preference = git_config_bool(var, value);
		return 0;
	}

	return 0;
}

void list_config_advices(struct string_list *list, const char *prefix)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(advice_config_keys); i++)
		list_config_item(list, prefix, advice_config_keys[i]);
}

int error_resolve_conflict(const char *me)
{
	if (!strcmp(me, "cherry-pick"))
		error(_("Cherry-picking is not possible because you have unmerged files."));
	else if (!strcmp(me, "commit"))
		error(_("Committing is not possible because you have unmerged files."));
	else if (!strcmp(me, "merge"))
		error(_("Merging is not possible because you have unmerged files."));
	else if (!strcmp(me, "pull"))
		error(_("Pulling is not possible because you have unmerged files."));
	else if (!strcmp(me, "revert"))
		error(_("Reverting is not possible because you have unmerged files."));
	else
		error(_("It is not possible to %s because you have unmerged files."),
			me);

	if (advice_resolve_conflict)
		/*
		 * Message used both when 'git commit' fails and when
		 * other commands doing a merge do.
		 */
		advise(_("Fix them up in the work tree, and then use 'git add/rm <file>'\n"
			 "as appropriate to mark resolution and make a commit."));
	return -1;
}

void NORETURN die_resolve_conflict(const char *me)
{
	error_resolve_conflict(me);
	die(_("Exiting because of an unresolved conflict."));
}

void NORETURN die_conclude_merge(void)
{
	error(_("You have not concluded your merge (MERGE_HEAD exists)."));
	if (advice_resolve_conflict)
		advise(_("Please, commit your changes before merging."));
	die(_("Exiting because of unfinished merge."));
}

void detach_advice(const char *new_name)
{
	const char *fmt =
	_("Note: switching to '%s'.\n"
	"\n"
	"You are in 'detached HEAD' state. You can look around, make experimental\n"
	"changes and commit them, and you can discard any commits you make in this\n"
	"state without impacting any branches by switching back to a branch.\n"
	"\n"
	"If you want to create a new branch to retain commits you create, you may\n"
	"do so (now or later) by using -c with the switch command. Example:\n"
	"\n"
	"  git switch -c <new-branch-name>\n"
	"\n"
	"Or undo this operation with:\n"
	"\n"
	"  git switch -\n"
	"\n"
	"Turn off this advice by setting config variable advice.detachedHead to false\n\n");

	fprintf(stderr, fmt, new_name);
}
