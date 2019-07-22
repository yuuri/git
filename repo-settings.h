#ifndef REPO_SETTINGS_H
#define REPO_SETTINGS_H

#define CORE_UNTRACKED_CACHE_WRITE (1 << 0)
#define CORE_UNTRACKED_CACHE_KEEP (1 << 1)

#define MERGE_DIRECTORY_RENAMES_CONFLICT 1
#define MERGE_DIRECTORY_RENAMES_TRUE 2

#define FETCH_NEGOTIATION_DEFAULT 1
#define FETCH_NEGOTIATION_SKIPPING 2

struct repo_settings {
	int core_commit_graph;
	int gc_write_commit_graph;

	int index_version;
	int core_untracked_cache;

	int pack_use_sparse;
	int merge_directory_renames;
	int fetch_negotiation_algorithm;
};

struct repository;

void prepare_repo_settings(struct repository *r);

#endif /* REPO_SETTINGS_H */
