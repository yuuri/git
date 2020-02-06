#ifndef ADVICE_H
#define ADVICE_H

#include "git-compat-util.h"

extern int advice_fetch_show_forced_updates;
extern int advice_push_update_rejected;
extern int advice_push_non_ff_current;
extern int advice_push_non_ff_matching;
extern int advice_push_already_exists;
extern int advice_push_fetch_first;
extern int advice_push_needs_force;
extern int advice_push_unqualified_ref_name;
extern int advice_status_hints;
extern int advice_status_u_option;
extern int advice_status_ahead_behind_warning;
extern int advice_commit_before_merge;
extern int advice_reset_quiet_warning;
extern int advice_resolve_conflict;
extern int advice_sequencer_in_use;
extern int advice_implicit_identity;
extern int advice_detached_head;
extern int advice_set_upstream_failure;
extern int advice_object_name_warning;
extern int advice_amworkdir;
extern int advice_rm_hints;
extern int advice_add_embedded_repo;
extern int advice_ignored_hook;
extern int advice_waiting_for_editor;
extern int advice_graft_file_deprecated;
extern int advice_checkout_ambiguous_remote_branch_name;
extern int advice_submodule_alternate_error_strategy_die;

/**
 To add a new advice, you need to:
 - Define an advice_type.
 - Add a new entry to advice_config_keys list.
 - Add the new config variable to Documentation/config/advice.txt.
 - Call advise_if_enabled to print your advice.
 */
enum advice_type {
	FETCH_SHOW_FORCED_UPDATES = 0,
	PUSH_UPDATE_REJECTED = 1,
	PUSH_UPDATE_REJECTED_ALIAS = 2,
	PUSH_NON_FF_CURRENT = 3,
	PUSH_NON_FF_MATCHING = 4,
	PUSH_ALREADY_EXISTS = 5,
	PUSH_FETCH_FIRST = 6,
	PUSH_NEEDS_FORCE = 7,
	PUSH_UNQUALIFIED_REF_NAME = 8,
	STATUS_HINTS = 9,
	STATUS_U_OPTION = 10,
	STATUS_AHEAD_BEHIND_WARNING = 11,
	COMMIT_BEFORE_MERGE = 12,
	RESET_QUIET_WARNING = 13,
	RESOLVE_CONFLICT = 14,
	SEQUENCER_IN_USE = 15,
	IMPLICIT_IDENTITY = 16,
	DETACHED_HEAD = 17,
	SET_UPSTREAM_FAILURE = 18,
	OBJECT_NAME_WARNING = 19,
	AMWORKDIR = 20,
	RM_HINTS = 21,
	ADD_EMBEDDED_REPO = 22,
	IGNORED_HOOK = 23,
	WAITING_FOR_EDITOR = 24,
	GRAFT_FILE_DEPRECATED = 25,
	CHECKOUT_AMBIGUOUS_REMOTE_BRANCH_NAME = 26,
	NESTED_TAG = 27,
	SUBMODULE_ALTERNATE_ERROR_STRATEGY_DIE = 28,
};


int git_default_advice_config(const char *var, const char *value);
__attribute__((format (printf, 1, 2)))
void advise(const char *advice, ...);

/**
 Checks if advice type is enabled (can be printed to the user).
 Should be called before advise().
 */
int advice_enabled(enum advice_type type);

/**
 Checks if PUSH_UPDATE_REJECTED advice type is enabled.
 */
int advice_push_update_rejected_enabled(void);

/**
 Checks the visibility of the advice before priniting.
 */
void advise_if_enabled(enum advice_type type, const char *advice, ...);

int error_resolve_conflict(const char *me);
void NORETURN die_resolve_conflict(const char *me);
void NORETURN die_conclude_merge(void);
void detach_advice(const char *new_name);

#endif /* ADVICE_H */
