#ifndef OBJECT_STORE_H
#define OBJECT_STORE_H

#include "cache.h"

struct object_store {
	struct alternate_object_database *alt_odb_list;
	struct alternate_object_database **alt_odb_tail;
};
#define OBJECT_STORE_INIT { NULL, NULL }

#endif /* OBJECT_STORE_H */
