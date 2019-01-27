#include "cache.h"
#include "metacommit-parser.h"
#include "commit.h"

/*
 * Search the commit buffer for a line starting with the given key. Unlike
 * find_commit_header, this also searches the commit message body.
 */
static const char *find_key(const char *msg, const char *key, size_t *out_len)
{
	int key_len = strlen(key);
	const char *line = msg;

	while (line) {
		const char *eol = strchrnul(line, '\n');

		if (eol - line > key_len &&
				!strncmp(line, key, key_len) &&
				line[key_len] == ' ') {
			*out_len = eol - line - key_len - 1;
			return line + key_len + 1;
		}
		line = *eol ? eol + 1 : NULL;
	}
	return NULL;
}

static struct commit *get_commit_by_index(struct commit_list *to_search, int index)
{
	while (to_search && index) {
		to_search = to_search->next;
		--index;
	}

	return to_search->item;
}

/*
 * Writes the content parent's object id to "content".
 * Returns the metacommit type. See the METACOMMIT_TYPE_* constants.
 */
int get_metacommit_content(
	struct commit *commit, struct object_id *content)
{
	const char *buffer = get_commit_buffer(commit, NULL);
	size_t parent_types_size;
	const char *parent_types = find_key(buffer, "parent-type",
		&parent_types_size);
	const char *end;
	int index = 0;
	int ret;
	struct commit *content_parent;

	if (!parent_types) {
		return METACOMMIT_TYPE_NONE;
	}

	end = &(parent_types[parent_types_size]);

	while (1) {
		char next = *parent_types;
		if (next == ' ') {
			index++;
		}
		if (next == 'c') {
			ret = METACOMMIT_TYPE_NORMAL;
			break;
		}
		if (next == 'a') {
			ret = METACOMMIT_TYPE_ABANDONED;
			break;
		}
		parent_types++;
		if (parent_types >= end) {
			return METACOMMIT_TYPE_NONE;
		}
	}

	content_parent = get_commit_by_index(commit->parents, index);

	if (!content_parent) {
		return METACOMMIT_TYPE_NONE;
	}

	oidcpy(content, &(content_parent->object.oid));
	return ret;
}
