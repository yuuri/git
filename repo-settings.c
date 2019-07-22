#include "cache.h"
#include "repository.h"
#include "config.h"
#include "repo-settings.h"

#define UPDATE_DEFAULT(s,v) do { if (s == -1) { s = v; } } while(0)

static int git_repo_config(const char *key, const char *value, void *cb)
{
	struct repo_settings *rs = (struct repo_settings *)cb;

	if (!strcmp(key, "feature.manycommits")) {
		UPDATE_DEFAULT(rs->core_commit_graph, 1);
		UPDATE_DEFAULT(rs->gc_write_commit_graph, 1);
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
	r->settings->pack_use_sparse = -1;
	r->settings->index_version = -1;
	r->settings->core_untracked_cache = -1;

	repo_config(r, git_repo_config, r->settings);

	/* Hack for test programs like test-dump-untracked-cache */
	if (ignore_untracked_cache_config)
		r->settings->core_untracked_cache = CORE_UNTRACKED_CACHE_KEEP;
	else
		UPDATE_DEFAULT(r->settings->core_untracked_cache, CORE_UNTRACKED_CACHE_KEEP);
}
