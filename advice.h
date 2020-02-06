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
extern int advice_nested_tag;
extern int advice_submodule_alternate_error_strategy_die;

/*
 * To add a new advice, you need to:
 * Define a new const char array.
 * Add a new entry to advice_config_keys list.
 * Add the new config variable to Documentation/config/advice.txt.
 * Call advise_if_enabled to print your advice.
 */
extern const char ADVICE_ADD_EMBEDDED_REPO[];
extern const char ADVICE_AM_WORK_DIR[];
extern const char ADVICE_CHECKOUT_AMBIGUOUS_REMOTE_BRANCH_NAME[];
extern const char ADVICE_COMMIT_BEFORE_MERGE[];
extern const char ADVICE_DETACHED_HEAD[];
extern const char ADVICE_FETCH_SHOW_FORCED_UPDATES[];
extern const char ADVICE_GRAFT_FILE_DEPRECATED[];
extern const char ADVICE_IGNORED_HOOK[];
extern const char ADVICE_IMPLICIT_IDENTITY[];
extern const char ADVICE_NESTED_TAG[];
extern const char ADVICE_OBJECT_NAME_WARNING[];
extern const char ADVICE_PUSH_ALREADY_EXISTS[];
extern const char ADVICE_PUSH_FETCH_FIRST[];
extern const char ADVICE_PUSH_NEEDS_FORCE[];
extern const char ADVICE_PUSH_NON_FF_CURRENT[];
extern const char ADVICE_PUSH_NON_FF_MATCHING[];
extern const char ADVICE_PUSH_UNQUALIFIED_REF_NAME[];
extern const char ADVICE_PUSH_UPDATE_REJECTED_ALIAS[];
extern const char ADVICE_PUSH_UPDATE_REJECTED[];
extern const char ADVICE_RESET_QUIET_WARNING[];
extern const char ADVICE_RESOLVE_CONFLICT[];
extern const char ADVICE_RM_HINTS[];
extern const char ADVICE_SEQUENCER_IN_USE[];
extern const char ADVICE_SET_UPSTREAM_FAILURE[];
extern const char ADVICE_STATUS_AHEAD_BEHIND_WARNING[];
extern const char ADVICE_STATUS_HINTS[];
extern const char ADVICE_STATUS_U_OPTION[];
extern const char ADVICE_SUBMODULE_ALTERNATE_ERROR_STRATEGY_DIE[];
extern const char ADVICE_WAITING_FOR_EDITOR[];

int git_default_advice_config(const char *var, const char *value);
__attribute__((format (printf, 1, 2)))
void advise(const char *advice, ...);

/**
 * Checks if advice type is enabled (can be printed to the user).
 * Should be called before advise().
 */
int advice_enabled(const char *advice_key);

/**
 * Checks the visibility of the advice before printing.
 */
void advise_if_enabled(const char *advice_key, const char *advice, ...);

int error_resolve_conflict(const char *me);
void NORETURN die_resolve_conflict(const char *me);
void NORETURN die_conclude_merge(void);
void detach_advice(const char *new_name);

#endif /* ADVICE_H */
