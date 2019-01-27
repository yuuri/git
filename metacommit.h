#ifndef METACOMMIT_H
#define METACOMMIT_H

// If specified, non-fast-forward changes are permitted.
#define UPDATE_OPTION_FORCE     0x0001
// If specified, no attempt will be made to append to existing changes.
// Normally, if a metacommit points to a commit in its replace or origin
// list and an existing change points to that same commit as its content, the
// new metacommit will attempt to append to that same change. This may replace
// the commit parent with one or more metacommits from the head of the appended
// changes. This option disables this behavior, and will always create a new
// change rather than reusing existing changes.
#define UPDATE_OPTION_NOAPPEND  0x0002

// Metacommit Data

struct metacommit_data {
	struct object_id content;
	struct oid_array replace;
	struct oid_array origin;
	int abandoned;
};

extern void init_metacommit_data(struct metacommit_data *state);

extern void clear_metacommit_data(struct metacommit_data *state);

extern int record_metacommit(struct repository *repo,
	const struct metacommit_data *metacommit,
	const char* override_change, int options, struct strbuf *err);

extern void modify_change(struct repository *repo,
	const struct object_id *old_commit, const struct object_id *new_commit,
	struct strbuf *err);

extern int write_metacommit(struct repository *repo, struct metacommit_data *state,
	struct object_id *result);

#endif
