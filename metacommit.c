#include "cache.h"
#include "metacommit.h"
#include "commit.h"
#include "change-table.h"
#include "refs.h"

void init_metacommit_data(struct metacommit_data *state)
{
	memset(state, 0, sizeof(*state));
}

void clear_metacommit_data(struct metacommit_data *state)
{
	oid_array_clear(&state->replace);
	oid_array_clear(&state->origin);
}

static void compute_default_change_name(struct commit *initial_commit,
	struct strbuf* result)
{
	const char *buffer = get_commit_buffer(initial_commit, NULL);
	const char *subject;
	const char *eol;
	int len;
	find_commit_subject(buffer, &subject);
	eol = strchrnul(subject, '\n');
	for (len = 0;subject < eol && len < 10; ++subject, ++len) {
		char next = *subject;
		if (isspace(next)) {
			continue;
		}

		strbuf_addch(result, next);
	}
}

/*
 * Computes a change name for a change rooted at the given initial commit. Good
 * change names should be memorable, unique, and easy to type. They are not
 * required to match the commit comment.
 */
static void compute_change_name(struct commit *initial_commit, struct strbuf* result)
{
	struct strbuf default_name;
	struct object_id unused;

	strbuf_init(&default_name, 0);
	if (initial_commit) {
		compute_default_change_name(initial_commit, &default_name);
	} else {
		strbuf_addstr(&default_name, "change");
	}
	strbuf_addstr(result, "refs/metas/");
	strbuf_addstr(result, default_name.buf);

	// If there is already a change of this name, append a suffix
	if (!read_ref(result->buf, &unused)) {
		int suffix = 2;
		int original_length = result->len;

		while (1) {
			strbuf_addf(result, "%d", suffix);
			if (read_ref(result->buf, &unused)) {
				break;
			}
			strbuf_remove(result, original_length, result->len - original_length);
			++suffix;
		}
	}

	strbuf_release(&default_name);
}

struct resolve_metacommit_callback_data
{
	struct change_table* active_changes;
	struct string_list *changes;
	struct oid_array *heads;
};

static int resolve_metacommit_callback(const char *refname, void *cb_data)
{
	struct resolve_metacommit_callback_data *data = (struct resolve_metacommit_callback_data *)cb_data;
	struct change_head *chhead;

	chhead = get_change_head(data->active_changes, refname);

	if (data->changes) {
		string_list_append(data->changes, refname)->util = &(chhead->head);
	}
	if (data->heads) {
		oid_array_append(data->heads, &(chhead->head));
	}

	return 0;
}

/*
 * Produces the final form of a metacommit based on the current change refs.
 */
static void resolve_metacommit(
	struct repository* repo,
	struct change_table* active_changes,
	const struct metacommit_data *to_resolve,
	struct metacommit_data *resolved_output,
	struct string_list *to_advance,
	int allow_append)
{
	int i;
	int len = to_resolve->replace.nr;
	struct resolve_metacommit_callback_data cbdata;
	int old_change_list_length = to_advance->nr;
	struct commit* content;

	oidcpy(&resolved_output->content, &to_resolve->content);

	// First look for changes that point to any of the replacement edges in the
	// metacommit. These will be the changes that get advanced by this metacommit.
	resolved_output->abandoned = to_resolve->abandoned;
	cbdata.active_changes = active_changes;
	cbdata.changes = to_advance;
	cbdata.heads = &(resolved_output->replace);

	if (allow_append) {
		for (i = 0; i < len; i++) {
			int old_number = resolved_output->replace.nr;
			for_each_change_referencing(active_changes, &(to_resolve->replace.oid[i]),
				resolve_metacommit_callback, &cbdata);
			// If no changes were found, use the unresolved value
			if (old_number == resolved_output->replace.nr) {
				oid_array_append(&(resolved_output->replace), &(to_resolve->replace.oid[i]));
			}
		}
	}

	cbdata.changes = NULL;
	cbdata.heads = &(resolved_output->origin);

	len = to_resolve->origin.nr;
	for (i = 0; i < len; i++) {
		int old_number = resolved_output->origin.nr;
		for_each_change_referencing(active_changes, &(to_resolve->origin.oid[i]),
			resolve_metacommit_callback, &cbdata);
		if (old_number == resolved_output->origin.nr) {
			oid_array_append(&(resolved_output->origin), &(to_resolve->origin.oid[i]));
		}
	}

	// If no changes were advanced by this metacommit, we'll need to create a new
	// one.
	if (to_advance->nr == old_change_list_length) {
		struct strbuf change_name;

		strbuf_init(&change_name, 80);
		content = lookup_commit_reference_gently(repo, &(to_resolve->content), 1);

		compute_change_name(content, &change_name);
		string_list_append(to_advance, change_name.buf);
		strbuf_release(&change_name);
	}
}

static void lookup_commits(
	struct repository *repo,
	struct oid_array *to_lookup,
	struct commit_list **result)
{
	int i = to_lookup->nr;

	while (--i >= 0) {
		struct object_id *next = &(to_lookup->oid[i]);
		struct commit *commit = lookup_commit_reference_gently(repo, next, 1);
		commit_list_insert(commit, result);
	}
}

#define PARENT_TYPE_PREFIX "parent-type "

/*
 * Creates a new metacommit object with the given content. Writes the object
 * id of the newly-created commit to result.
 */
int write_metacommit(struct repository *repo, struct metacommit_data *state,
	struct object_id *result)
{
	struct commit_list *parents = NULL;
	struct strbuf comment;
	int i;
	struct commit *content;

	strbuf_init(&comment, strlen(PARENT_TYPE_PREFIX)
		+ 1 + 2 * (state->origin.nr + state->replace.nr));
	lookup_commits(repo, &state->origin, &parents);
	lookup_commits(repo, &state->replace, &parents);
	content = lookup_commit_reference_gently(repo, &state->content, 1);
	if (!content) {
		strbuf_release(&comment);
		free_commit_list(parents);
		return -1;
	}
	commit_list_insert(content, &parents);

	strbuf_addstr(&comment, PARENT_TYPE_PREFIX);
	strbuf_addstr(&comment, state->abandoned ? "a" : "c");
	for (i = 0; i < state->replace.nr; i++) {
		strbuf_addstr(&comment, " r");
	}

	for (i = 0; i < state->origin.nr; i++) {
		strbuf_addstr(&comment, " o");
	}

	// The parents list will be freed by this call
	commit_tree(comment.buf, comment.len, repo->hash_algo->empty_tree, parents,
		result, NULL, NULL);

	strbuf_release(&comment);
	return 0;
}

/*
 * Returns true iff the given metacommit is abandoned, has one or more origin
 * parents, or has one or more replacement parents.
 */
static int is_nontrivial_metacommit(struct metacommit_data *state)
{
	return state->replace.nr || state->origin.nr || state->abandoned;
}

/*
 * Records the relationships described by the given metacommit in the
 * repository.
 *
 * If override_change is NULL (the default), an attempt will be made
 * to append to existing changes wherever possible instead of creating new ones.
 * If override_change is non-null, only the given change ref will be updated.
 *
 * options is a bitwise combination of the UPDATE_OPTION_* flags.
 */
int record_metacommit(struct repository *repo,
	const struct metacommit_data *metacommit,
	const char* override_change, int options, struct strbuf *err)
{
	static const char *msg = "updating change";
	struct metacommit_data resolved_metacommit;
	struct string_list changes;
	struct object_id commit_target;
	struct ref_transaction *transaction = NULL;
	struct object_id old_head_working;
	const struct object_id *old_head;
	struct change_table chtable;
	int i;
	int ret = 0;
	int force = (options & UPDATE_OPTION_FORCE);

	init_metacommit_data(&resolved_metacommit);
	string_list_init(&changes, 1);

	change_table_init(&chtable);

	change_table_add_all_visible(&chtable, repo);

	resolve_metacommit(repo, &chtable, metacommit, &resolved_metacommit, &changes,
		(options & UPDATE_OPTION_NOAPPEND) == 0);

	if (override_change) {
		old_head = &old_head_working;
		string_list_clear(&changes, 0);
		if (get_oid_committish(override_change, &old_head_working)) {
			// ...then this is a newly-created change
			old_head = &null_oid;
		} else if (!force) {
			if (!oid_array_readonly_contains(&(resolved_metacommit.replace),
				&old_head_working)) {
				// Attempted non-fast-forward change
				strbuf_addf(err, _("non-fast-forward update to '%s'"),
					override_change);
				ret = -1;
				goto cleanup;
			}
		}
		// The expected "current" head of the change is stored in the util pointer
		string_list_append(&changes, override_change)->util = (void*)old_head;
	}

	if (is_nontrivial_metacommit(&resolved_metacommit)) {
		// If there are any origin or replacement parents, create a new metacommit
		// object.
		if (write_metacommit(repo, &resolved_metacommit, &commit_target) < 0) {
			ret = -1;
			goto cleanup;
		}
	} else {
		// If the metacommit would only contain a content commit, point to the
		// commit itself rather than creating a trivial metacommit.
		oidcpy(&commit_target, &(resolved_metacommit.content));
	}

	// If a change already exists with this target and we're not forcing an
	// update to some specific override_change && change, there's nothing to do.
	if (!override_change &&
		change_table_has_change_referencing(&chtable, &commit_target)) {
		// Not an error
		goto cleanup;
	}

	transaction = ref_transaction_begin(err);

	// Update the refs for each affected change
	if (!transaction) {
		ret = -1;
	} else {
		for (i = 0; i < changes.nr; i++) {
			struct string_list_item *it = &(changes.items[i]);

			// The expected current head of the change is stored in the util pointer.
			// It is null if the change should be newly-created.
			if (it->util) {
				if (ref_transaction_update(transaction, it->string, &commit_target,
					force ? NULL : it->util, 0, msg, err)) {

					ret = -1;
				}
			} else {
				if (ref_transaction_create(transaction, it->string,
					&commit_target, 0, msg, err)) {

					ret = -1;
				}
			}
		}

		if (!ret) {
			if (ref_transaction_commit(transaction, err)) {
				ret = -1;
			}
		}
	}

cleanup:
	ref_transaction_free(transaction);
	string_list_clear(&changes, 0);
	clear_metacommit_data(&resolved_metacommit);
	change_table_clear(&chtable);
	return ret;
}

/*
 * Should be invoked after a command that has "modify" semantics - commands that
 * create a new commit based on an old commit and treat the new one as a
 * replacement for the old one. This method records the replacement in the
 * change graph, such that a future evolve operation will rebase children of
 * the old commit onto the new commit.
 */
void modify_change(
	struct repository *repo,
	const struct object_id *old_commit,
	const struct object_id *new_commit,
	struct strbuf *err)
{
	struct metacommit_data metacommit;

	init_metacommit_data(&metacommit);
	oidcpy(&(metacommit.content), new_commit);
	oid_array_append(&(metacommit.replace), old_commit);

	record_metacommit(repo, &metacommit, NULL, 0, err);

	clear_metacommit_data(&metacommit);
}
