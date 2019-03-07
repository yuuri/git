#ifndef ADD_INTERACTIVE_H
#define ADD_INTERACTIVE_H

int add_i_config(const char *var, const char *value, void *cb);

struct repository;
struct pathspec;
int run_add_i(struct repository *r, const struct pathspec *ps);

#endif
