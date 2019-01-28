#include "cache.h"
#include "change-table.h"
#include "commit.h"
#include "ref-filter.h"
#include "metacommit-parser.h"

void change_table_init(struct change_table *to_initialize)
{
	memset(to_initialize, 0, sizeof(*to_initialize));
	mem_pool_init(&(to_initialize->memory_pool), 0);
	to_initialize->memory_pool->block_alloc = 4*1024 - sizeof(struct mp_block);
	oidmap_init(&(to_initialize->oid_to_metadata_index), 0);
	string_list_init(&(to_initialize->refname_to_change_head), 1);
}

static void change_list_clear(struct change_list *to_clear) {
	string_list_clear(&to_clear->additional_refnames, 0);
}

static void commit_change_list_entry_clear(
	struct commit_change_list_entry *to_clear) {
	change_list_clear(&(to_clear->changes));
}

static void change_head_array_clear(struct change_head_array *to_clear)
{
	FREE_AND_NULL(to_clear->array);
}

void change_table_clear(struct change_table *to_clear)
{
	struct oidmap_iter iter;
	struct commit_change_list_entry *next;
	for (next = oidmap_iter_first(&to_clear->oid_to_metadata_index, &iter);
		next;
		next = oidmap_iter_next(&iter)) {

		commit_change_list_entry_clear(next);
	}

	oidmap_free(&to_clear->oid_to_metadata_index, 0);
	string_list_clear(&(to_clear->refname_to_change_head), 0);
	change_head_array_clear(&to_clear->heads);
	mem_pool_discard(to_clear->memory_pool, 0);
}

/*
 * Appends a new, empty, change_head struct to the end of the given array.
 * Returns the index of the newly-added struct.
 */
static int change_head_array_append(struct change_head_array *to_add)
{
	int index = to_add->nr++;
	struct change_head *new_head;
	ALLOC_GROW(to_add->array, to_add->nr, to_add->alloc);
	new_head = &(to_add->array[index]);
	memset(new_head, 0, sizeof(*new_head));
	return index;
}

static void add_head_to_commit(struct change_table *to_modify,
	const struct object_id *to_add, const char *refname)
{
	struct commit_change_list_entry *entry;

	// Note: the indices in the map are 1-based. 0 is used to indicate a missing
	// element.
	entry = oidmap_get(&(to_modify->oid_to_metadata_index), to_add);
	if (!entry) {
		entry = mem_pool_calloc(to_modify->memory_pool, 1,
			sizeof(*entry));
		oidcpy(&entry->entry.oid, to_add);
		oidmap_put(&(to_modify->oid_to_metadata_index), entry);
		string_list_init(&(entry->changes.additional_refnames), 0);
	}

	if (entry->changes.first_refname == NULL) {
		entry->changes.first_refname = refname;
	} else {
		string_list_insert(&entry->changes.additional_refnames, refname);
	}
}

void change_table_add(struct change_table *to_modify, const char *refname,
	struct commit *to_add)
{
	struct change_head *new_head;
	struct string_list_item *new_item;
	long index;
	int metacommit_type;

	index = change_head_array_append(&to_modify->heads);
	new_head = &(to_modify->heads.array[index]);

	oidcpy(&new_head->head, &(to_add->object.oid));

	metacommit_type = get_metacommit_content(to_add, &new_head->content);
	if (metacommit_type == METACOMMIT_TYPE_NONE) {
		oidcpy(&new_head->content, &(to_add->object.oid));
	}
	new_head->abandoned = (metacommit_type == METACOMMIT_TYPE_ABANDONED);
	new_head->remote = starts_with(refname, "refs/remote/");
	new_head->hidden = starts_with(refname, "refs/hiddenmetas/");

	new_item = string_list_insert(&to_modify->refname_to_change_head, refname);
	new_item->util = (void*)index;
	// Use pointers to the copy of the string we're retaining locally
	refname = new_item->string;

	if (!oideq(&new_head->content, &new_head->head)) {
		add_head_to_commit(to_modify, &(new_head->content), refname);
	}
	add_head_to_commit(to_modify, &(new_head->head), refname);
}

void change_table_add_all_visible(struct change_table *to_modify,
	struct repository* repo)
{
	struct ref_filter filter;
	const char *name_patterns[] = {NULL};
	memset(&filter, 0, sizeof(filter));
	filter.kind = FILTER_REFS_CHANGES;
	filter.name_patterns = name_patterns;

	change_table_add_matching_filter(to_modify, repo, &filter);
}

void change_table_add_matching_filter(struct change_table *to_modify,
	struct repository* repo, struct ref_filter *filter)
{
	struct ref_array matching_refs;
	int i;

	memset(&matching_refs, 0, sizeof(matching_refs));
	filter_refs(&matching_refs, filter, filter->kind);

	// Determine the object id for the latest content commit for each change.
	// Fetch the commit at the head of each change ref. If it's a normal commit,
	// that's the commit we want. If it's a metacommit, locate its content parent
	// and use that.

	for (i = 0; i < matching_refs.nr; i++) {
		struct ref_array_item *item = matching_refs.items[i];
		struct commit *commit = item->commit;

		commit = lookup_commit_reference_gently(repo, &(item->objectname), 1);

		if (commit != NULL) {
			change_table_add(to_modify, item->refname, commit);
		}
	}

	ref_array_clear(&matching_refs);
}

static int return_true_callback(const char *refname, void *cb_data)
{
	return 1;
}

int change_table_has_change_referencing(struct change_table *changes,
	const struct object_id *referenced_commit_id)
{
	return for_each_change_referencing(changes, referenced_commit_id,
		return_true_callback, NULL);
}

int for_each_change_referencing(struct change_table *table,
	const struct object_id *referenced_commit_id, each_change_fn fn, void *cb_data)
{
	const struct change_list *changes;
	int i;
	int retvalue;
	struct commit_change_list_entry *entry;

	entry = oidmap_get(&table->oid_to_metadata_index,
		referenced_commit_id);
	// If this commit isn't referenced by any changes, it won't be in the map
	if (!entry) {
		return 0;
	}
	changes = &(entry->changes);
	if (changes->first_refname == NULL) {
		return 0;
	}
	retvalue = fn(changes->first_refname, cb_data);
	for (i = 0; retvalue == 0 && i < changes->additional_refnames.nr; i++) {
		retvalue = fn(changes->additional_refnames.items[i].string, cb_data);
	}
	return retvalue;
}

struct change_head* get_change_head(struct change_table *heads,
	const char* refname)
{
	struct string_list_item *item = string_list_lookup(
		&heads->refname_to_change_head, refname);
	long index;

	if (!item) {
		return NULL;
	}

	index = (long)item->util;
	return &(heads->heads.array[index]);
}
