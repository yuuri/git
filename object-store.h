#ifndef OBJECT_STORE_H
#define OBJECT_STORE_H

#include "cache.h"
#include "mru.h"

struct object_store {
	struct packed_git *packed_git;

	/*
	 * A most-recently-used ordered version of the packed_git list, which can
	 * be iterated instead of packed_git (and marked via mru_mark).
	 */
	struct mru packed_git_mru;

	struct alternate_object_database *alt_odb_list;
	struct alternate_object_database **alt_odb_tail;
};
#define OBJECT_STORE_INIT { NULL, MRU_INIT, NULL, NULL }

struct packed_git {
	struct packed_git *next;
	struct pack_window *windows;
	off_t pack_size;
	const void *index_data;
	size_t index_size;
	uint32_t num_objects;
	uint32_t num_bad_objects;
	unsigned char *bad_object_sha1;
	int index_version;
	time_t mtime;
	int pack_fd;
	unsigned pack_local:1,
		 pack_keep:1,
		 freshened:1,
		 do_not_close:1;
	unsigned char sha1[20];
	struct revindex_entry *revindex;
	/* something like ".git/objects/pack/xxxxx.pack" */
	char pack_name[FLEX_ARRAY]; /* more */
};

#endif /* OBJECT_STORE_H */
