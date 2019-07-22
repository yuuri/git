#include "cache.h"
#include "repository.h"
#include "config.h"
#include "repo-settings.h"

#define UPDATE_DEFAULT(s,v) do { if (s == -1) { s = v; } } while(0)

static int git_repo_config(const char *key, const char *value, void *cb)
{
	struct repo_settings *rs = (struct repo_settings *)cb;

	if (!strcmp(key, "feature.experimental")) {
		UPDATE_DEFAULT(rs->pack_use_sparse, 1);
		UPDATE_DEFAULT(rs->merge_directory_renames, MERGE_DIRECTORY_RENAMES_TRUE);
		UPDATE_DEFAULT(rs->fetch_negotiation_algorithm, FETCH_NEGOTIATION_SKIPPING);
		return 0;
	}
	if (!strcmp(key, "feature.manycommits")) {
		UPDATE_DEFAULT(rs->core_commit_graph, 1);
		UPDATE_DEFAULT(rs->gc_write_commit_graph, 1);
		return 0;
	}
	if (!strcmp(key, "feature.manyfiles")) {
		UPDATE_DEFAULT(rs->index_version, 4);
		UPDATE_DEFAULT(rs->core_untracked_cache,
			       CORE_UNTRACKED_CACHE_KEEP | CORE_UNTRACKED_CACHE_WRITE);
		return 0;
	}
	if (!strcmp(key, "core.commitgraph")) {
		rs->core_commit_graph = git_config_bool(key, value);
		return 0;
	}
	if (!strcmp(key, "gc.writecommitgraph")) {
		rs->gc_write_commit_graph = git_config_bool(key, value);
		return 0;
	}
	if (!strcmp(key, "pack.usesparse")) {
		rs->pack_use_sparse = git_config_bool(key, value);
		return 0;
	}
	if (!strcmp(key, "index.version")) {
		rs->index_version = git_config_int(key, value);
		return 0;
	}
	if (!strcmp(key, "core.untrackedcache")) {
		int bool_value = git_parse_maybe_bool(value);
		if (bool_value == 0)
			rs->core_untracked_cache = 0;
		else if (bool_value == 1)
			rs->core_untracked_cache = CORE_UNTRACKED_CACHE_KEEP |
						   CORE_UNTRACKED_CACHE_WRITE;
		else if (!strcasecmp(value, "keep"))
			rs->core_untracked_cache = CORE_UNTRACKED_CACHE_KEEP;
		else
			error(_("unknown core.untrackedCache value '%s'; "
				"using 'keep' default value"), value);
		return 0;
	}
	if (!strcmp(key, "merge.directoryrenames")) {
		int bool_value = git_parse_maybe_bool(value);
		if (0 <= bool_value) {
			rs->merge_directory_renames = bool_value ? MERGE_DIRECTORY_RENAMES_TRUE : 0;
		} else if (!strcasecmp(value, "conflict")) {
			rs->merge_directory_renames = MERGE_DIRECTORY_RENAMES_CONFLICT;
		}
		return 0;
	}
	if (!strcmp(key, "fetch.negotiationalgorithm"))	{
		if (!strcasecmp(value, "skipping")) {
			rs->fetch_negotiation_algorithm = FETCH_NEGOTIATION_SKIPPING;
		} else {
			rs->fetch_negotiation_algorithm = FETCH_NEGOTIATION_DEFAULT;
		}
		return 0;
	}

	return 1;
}

void prepare_repo_settings(struct repository *r)
{
	if (r->settings)
		return;

	r->settings = xmalloc(sizeof(*r->settings));

	/* Defaults */
	r->settings->core_commit_graph = -1;
	r->settings->gc_write_commit_graph = -1;

	r->settings->index_version = -1;
	r->settings->core_untracked_cache = -1;

	r->settings->pack_use_sparse = -1;
	r->settings->merge_directory_renames = -1;
	r->settings->fetch_negotiation_algorithm = -1;

	repo_config(r, git_repo_config, r->settings);

	/* Hack for test programs like test-dump-untracked-cache */
	if (ignore_untracked_cache_config)
		r->settings->core_untracked_cache = CORE_UNTRACKED_CACHE_KEEP;
	else
		UPDATE_DEFAULT(r->settings->core_untracked_cache, CORE_UNTRACKED_CACHE_KEEP);

	UPDATE_DEFAULT(r->settings->merge_directory_renames, MERGE_DIRECTORY_RENAMES_CONFLICT);
	UPDATE_DEFAULT(r->settings->fetch_negotiation_algorithm, FETCH_NEGOTIATION_DEFAULT);
}
