#include "cache.h"
#include "add-interactive.h"
#include "config.h"

int add_i_config(const char *var, const char *value, void *cb)
{
	return git_default_config(var, value, cb);
}

int run_add_i(struct repository *r, const struct pathspec *ps)
{
	die(_("No commands are available in the built-in `git add -i` yet!"));
}
