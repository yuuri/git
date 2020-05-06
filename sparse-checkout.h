#ifndef SPARSE_CHECKOUT_H
#define SPARSE_CHECKOUT_H

#include "cache.h"
#include "repository.h"

#define SPARSE_CHECKOUT_DIR "sparse.dir"
#define SPARSE_CHECKOUT_INHERIT "sparse.inherit"
#define SPARSE_CHECKOUT_IN_TREE "sparse.intree"

struct pattern_list;

char *get_sparse_checkout_filename(void);
int populate_sparse_checkout_patterns(struct pattern_list *pl);
void write_patterns_to_file(FILE *fp, struct pattern_list *pl);
int update_working_directory(struct pattern_list *pl);
int update_in_tree_sparse_checkout(void);
int write_patterns(struct pattern_list *pl, int and_update);
int write_patterns_and_update(struct pattern_list *pl);
void insert_recursive_pattern(struct pattern_list *pl, struct strbuf *path);
void strbuf_to_cone_pattern(struct strbuf *line, struct pattern_list *pl);

int load_in_tree_list_from_config(struct repository *r,
				  struct string_list *sl);
int load_in_tree_pattern_list(struct repository *r,
			      struct string_list *sl,
			      struct pattern_list *pl);
int set_sparse_in_tree_config(struct repository *r, struct string_list *sl);

#endif
