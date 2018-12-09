#ifndef __BACKUP_LOG_H__
#define __BACKUP_LOG_H__

struct object_id;
struct strbuf;

void bkl_append(struct strbuf *output, const char *path,
		const struct object_id *from,
		const struct object_id *to);

int bkl_write(const char *path, struct strbuf *new_log);

#endif
