#ifndef REPO_SETTINGS_H
#define REPO_SETTINGS_H

enum untracked_cache_setting {
	UNTRACKED_CACHE_UNSET = -1,
	UNTRACKED_CACHE_REMOVE = 0,
	UNTRACKED_CACHE_KEEP = 1,
	UNTRACKED_CACHE_WRITE = 2
};

struct repo_settings {
	int core_commit_graph;
	int gc_write_commit_graph;

	int index_version;
	enum untracked_cache_setting core_untracked_cache;

	int pack_use_sparse;
};

struct repository;

void prepare_repo_settings(struct repository *r);

#endif /* REPO_SETTINGS_H */
