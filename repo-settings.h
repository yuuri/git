#ifndef REPO_SETTINGS_H
#define REPO_SETTINGS_H

enum untracked_cache_setting {
	UNTRACKED_CACHE_UNSET = -1,
	UNTRACKED_CACHE_REMOVE = 0,
	UNTRACKED_CACHE_KEEP = 1,
	UNTRACKED_CACHE_WRITE = 2
};

enum merge_directory_renames_setting {
	MERGE_DIRECTORY_RENAMES_UNSET = -1,
	MERGE_DIRECTORY_RENAMES_NONE = 0,
	MERGE_DIRECTORY_RENAMES_CONFLICT = 1,
	MERGE_DIRECTORY_RENAMES_TRUE = 2,
};

enum fetch_negotiation_setting {
	FETCH_NEGOTIATION_UNSET = -1,
	FETCH_NEGOTIATION_NONE = 0,
	FETCH_NEGOTIATION_DEFAULT = 1,
	FETCH_NEGOTIATION_SKIPPING = 2,
};

struct repo_settings {
	int core_commit_graph;
	int gc_write_commit_graph;

	int index_version;
	enum untracked_cache_setting core_untracked_cache;

	int pack_use_sparse;
	enum merge_directory_renames_setting merge_directory_renames;
	enum fetch_negotiation_setting fetch_negotiation_algorithm;
};

struct repository;

void prepare_repo_settings(struct repository *r);

#endif /* REPO_SETTINGS_H */
