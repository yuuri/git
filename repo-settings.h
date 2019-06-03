#ifndef REPO_SETTINGS_H
#define REPO_SETTINGS_H

struct repo_settings {
	char core_commit_graph;
	char gc_write_commit_graph;
	char pack_use_sparse;
	char status_ahead_behind;
	char fetch_show_forced_updates;
	int index_version;
};

struct repository;

void prepare_repo_settings(struct repository *r);

#endif /* REPO_SETTINGS_H */
