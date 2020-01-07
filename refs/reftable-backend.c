#include "../cache.h"
#include "../config.h"
#include "../refs.h"
#include "refs-internal.h"
#include "../iterator.h"
#include "../lockfile.h"
#include "../chdir-notify.h"

#include "../reftable/reftable.h"

extern struct ref_storage_be refs_be_reftable;

struct reftable_ref_store {
	struct ref_store base;
	unsigned int store_flags;

	int err;
	char *reftable_dir;
	char *table_list_file;
	struct stack *stack;
};

static void clear_log_record(struct log_record* log) {
        log->old_hash = NULL;
        log->new_hash = NULL;
        log->message = NULL;
        log->ref_name = NULL;
        log_record_clear(log);
}

static void fill_log_record(struct log_record* log) {
        const char* info = git_committer_info(0);
        struct ident_split split = {};
        int result = split_ident_line(&split, info, strlen(info));
        int sign = 1;
        assert(0==result);

        log_record_clear(log);
        log->name = xstrndup(split.name_begin, split.name_end - split.name_begin);
        log->email = xstrndup(split.mail_begin, split.mail_end - split.mail_begin);
        log->time = atol(split.date_begin);
        if (*split.tz_begin == '-') {
                sign = -1;
                split.tz_begin ++;
        }
        if (*split.tz_begin == '+') {
                sign = 1;
                split.tz_begin ++;
        }

        log->tz_offset = sign * atoi(split.tz_begin);
}

static struct ref_store *reftable_ref_store_create(const char *path,
                                                   unsigned int store_flags) {
  	struct reftable_ref_store *refs = xcalloc(1, sizeof(*refs));
	struct ref_store *ref_store = (struct ref_store *)refs;
	struct write_options cfg = {
		.block_size = 4096,
	};
	struct strbuf sb = STRBUF_INIT;

	base_ref_store_init(ref_store, &refs_be_reftable);
	refs->store_flags = store_flags;

	strbuf_addf(&sb, "%s/refs", path);
	refs->table_list_file = xstrdup(sb.buf);
	strbuf_reset(&sb);
	strbuf_addf(&sb, "%s/reftable", path);
	refs->reftable_dir = xstrdup(sb.buf);
	strbuf_release(&sb);

	refs->err = new_stack(&refs->stack, refs->reftable_dir, refs->table_list_file, cfg);

	return ref_store;
}

static int reftable_init_db(struct ref_store *ref_store, struct strbuf *err)
{
	struct reftable_ref_store *refs = (struct reftable_ref_store*) ref_store;
	FILE *f = fopen(refs->table_list_file, "a");
	if (f == NULL) {
	  return -1;
	}
	fclose(f);

	safe_create_dir(refs->reftable_dir, 1);
	return 0;
}


struct reftable_iterator {
	struct ref_iterator base;
	struct iterator iter;
	struct ref_record ref;
	struct object_id oid;
        struct ref_store *ref_store;
	char *prefix;
};

static int reftable_ref_iterator_advance(struct ref_iterator *ref_iterator)
{
        while (1) {
                struct reftable_iterator *ri = (struct reftable_iterator*) ref_iterator;
                int err = iterator_next_ref(ri->iter, &ri->ref);
                if (err > 0) {
                        return ITER_DONE;
                }
                if (err < 0) {
                        return ITER_ERROR;
                }

                ri->base.refname = ri->ref.ref_name;
                if (ri->prefix != NULL && 0 != strncmp(ri->prefix, ri->ref.ref_name, strlen(ri->prefix))) {
                        return ITER_DONE;
                }

                ri->base.flags = 0;
                if (ri->ref.value != NULL) {
                        memcpy(ri->oid.hash, ri->ref.value, GIT_SHA1_RAWSZ);
                } else if (ri->ref.target != NULL) {
                        int out_flags = 0;
                        const char *resolved =
                                refs_resolve_ref_unsafe(ri->ref_store, ri->ref.ref_name, RESOLVE_REF_READING,
                                                        &ri->oid, &out_flags);
                        ri->base.flags = out_flags;
                        if (resolved == NULL && !(ri->base.flags & DO_FOR_EACH_INCLUDE_BROKEN)
                            && (ri->base.flags & REF_ISBROKEN)) {
                                continue;
                        }
                }
                ri->base.oid  = &ri->oid;
                return ITER_OK;
        }
}

static int reftable_ref_iterator_peel(struct ref_iterator *ref_iterator,
				      struct object_id *peeled)
{
	struct reftable_iterator *ri = (struct reftable_iterator*) ref_iterator;
	if (ri->ref.target_value != NULL) {
		memcpy(peeled->hash, ri->ref.target_value, GIT_SHA1_RAWSZ);
		return 0;
	}

	return -1;
}

static int reftable_ref_iterator_abort(struct ref_iterator *ref_iterator)
{
	struct reftable_iterator *ri = (struct reftable_iterator*) ref_iterator;
	ref_record_clear(&ri->ref);
	iterator_destroy(&ri->iter);
	return 0;
}

static struct ref_iterator_vtable reftable_ref_iterator_vtable = {
	reftable_ref_iterator_advance,
	reftable_ref_iterator_peel,
	reftable_ref_iterator_abort
};

static struct ref_iterator *reftable_ref_iterator_begin(
		struct ref_store *ref_store,
		const char *prefix, unsigned int flags)
{
	struct reftable_ref_store *refs = (struct reftable_ref_store*) ref_store;
	struct reftable_iterator *ri = xcalloc(1, sizeof(*ri));
	struct merged_table *mt = NULL;
	int err = 0;
	if (refs->err) {
		// XXX ?
		return NULL;
	}

	mt = stack_merged_table(refs->stack);

	// XXX something with flags?
	err = merged_table_seek_ref(mt, &ri->iter, prefix);
	// XXX what to do with err?
	assert(err == 0);
	base_ref_iterator_init(&ri->base, &reftable_ref_iterator_vtable, 1);
	ri->base.oid = &ri->oid;
        ri->ref_store = ref_store;
	return &ri->base;
}

static int reftable_transaction_prepare(struct ref_store *ref_store,
					struct ref_transaction *transaction,
					struct strbuf *err)
{
	return 0;
}

static int reftable_transaction_abort(struct ref_store *ref_store,
				    struct ref_transaction *transaction,
				    struct strbuf *err)
{
	struct reftable_ref_store *refs = (struct reftable_ref_store*) ref_store;
	(void)refs;
	return 0;
}


static int ref_update_cmp(const void *a, const void *b) {
	return strcmp(((struct ref_update*)a)->refname, ((struct ref_update*)b)->refname);
}

static int reftable_check_old_oid(struct ref_store *refs, const char *refname, struct object_id *want_oid) {
        struct object_id out_oid = {};
        int out_flags = 0;
        const char *resolved =
                refs_resolve_ref_unsafe(refs, refname, RESOLVE_REF_READING,
                                        &out_oid, &out_flags);
        if (is_null_oid(want_oid) != (resolved == NULL)) {
                return LOCK_ERROR;
        }

        if (resolved != NULL && !oideq(&out_oid, want_oid)) {
                return LOCK_ERROR;
        }

        return 0;
}

static int write_transaction_table(struct writer *writer, void *arg) {
	struct ref_transaction *transaction = (struct ref_transaction *)arg;
	struct reftable_ref_store *refs
		= (struct reftable_ref_store*)transaction->ref_store;
	uint64_t ts = stack_next_update_index(refs->stack);
	int err = 0;
	// XXX - are we allowed to mutate the input data?
	qsort(transaction->updates, transaction->nr, sizeof(struct ref_update*),
	      ref_update_cmp);
	writer_set_limits(writer, ts, ts);

	for (int i = 0; i <  transaction->nr; i++) {
		struct ref_update * u = transaction->updates[i];
		if (u->flags & REF_HAVE_OLD) {
			err = reftable_check_old_oid(transaction->ref_store, u->refname, &u->old_oid);
			if (err < 0) {
				goto exit;
			}
		}
	}

	for (int i = 0; i <  transaction->nr; i++) {
		struct ref_update * u = transaction->updates[i];
		if (u->flags & REF_HAVE_NEW) {
                        struct object_id out_oid = {};
                        int out_flags = 0;
                        // XXX who owns the memory here?
                        const char *resolved
                                = refs_resolve_ref_unsafe(transaction->ref_store, u->refname, 0, &out_oid, &out_flags);
                        struct ref_record ref = {};
                        ref.ref_name = (char*)(resolved ? resolved : u->refname);
                        ref.value = u->new_oid.hash;
                        ref.update_index = ts;
                        err = writer_add_ref(writer, &ref);
                        if (err < 0) {
                                goto exit;
                        }
		}
	}


	for (int i = 0; i <  transaction->nr; i++) {
		struct ref_update * u = transaction->updates[i];
                struct log_record log = {};
                fill_log_record(&log);

                log.ref_name = (char*)u->refname;
                log.old_hash = u->old_oid.hash;
                log.new_hash = u->new_oid.hash;
                log.update_index = ts;
                log.message = u->msg;

                err = writer_add_log(writer, &log);
                clear_log_record(&log);
                if (err < 0) { return err; }
        }
exit:
	return err;
}

static int reftable_transaction_commit(struct ref_store *ref_store,
				       struct ref_transaction *transaction,
				       struct strbuf *errmsg)
{
	struct reftable_ref_store *refs = (struct reftable_ref_store*) ref_store;
	int err = stack_add(refs->stack,
		  &write_transaction_table,
		  transaction);
	if (err < 0) {
		strbuf_addf(errmsg, "reftable: transaction failure %s", error_str(err));
		return -1;
 	}

	return 0;
}


static int reftable_transaction_finish(struct ref_store *ref_store,
				     struct ref_transaction *transaction,
				     struct strbuf *err)
{
        return reftable_transaction_commit(ref_store, transaction, err);
}



struct write_delete_refs_arg {
	struct stack *stack;
	struct string_list *refnames;
	const char *logmsg;
        unsigned int flags;
};

static int write_delete_refs_table(struct writer *writer, void *argv) {
	struct write_delete_refs_arg *arg = (struct write_delete_refs_arg*)argv;
	uint64_t ts = stack_next_update_index(arg->stack);
	int err = 0;

        writer_set_limits(writer, ts, ts);
        for (int i = 0; i < arg->refnames->nr; i++) {
                struct ref_record ref = {
                        .ref_name = (char*) arg->refnames->items[i].string,
                        .update_index = ts,
                };
                err = writer_add_ref(writer, &ref);
                if (err < 0) {
                        return err;
                }
        }

        for (int i = 0; i < arg->refnames->nr; i++) {
                struct log_record log = {};
                fill_log_record(&log);
                log.message = xstrdup(arg->logmsg);
                log.new_hash = NULL;

                // XXX should lookup old oid.
                log.old_hash = NULL;
                log.update_index = ts;
                log.ref_name = (char*) arg->refnames->items[i].string;

                err = writer_add_log(writer, &log);
                clear_log_record(&log);
                if (err < 0) {
                        return err;
                }
        }
	return 0;
}

static int reftable_delete_refs(struct ref_store *ref_store, const char *msg,
                                struct string_list *refnames, unsigned int flags)
{
	struct reftable_ref_store *refs = (struct reftable_ref_store*)ref_store;
	struct write_delete_refs_arg arg = {
		.stack = refs->stack,
		.refnames = refnames,
		.logmsg = msg,
                .flags = flags,
	};
	return stack_add(refs->stack, &write_delete_refs_table, &arg);
}

static int reftable_pack_refs(struct ref_store *ref_store, unsigned int flags)
{
	struct reftable_ref_store *refs = (struct reftable_ref_store*)ref_store;
        // XXX reflog expiry.
	return stack_compact_all(refs->stack, NULL);
}

struct write_create_symref_arg {
	struct stack *stack;
	const char *refname;
	const char *target;
	const char *logmsg;
};

static int write_create_symref_table(struct writer *writer, void *arg) {
	struct write_create_symref_arg *create = (struct write_create_symref_arg*)arg;
	uint64_t ts = stack_next_update_index(create->stack);
	int err = 0;

	struct ref_record ref = {
		.ref_name = (char*) create->refname,
		.target = (char*) create->target,
		.update_index = ts,
	};
	writer_set_limits(writer, ts, ts);
	err = writer_add_ref(writer, &ref);
	if (err < 0) {
		return err;
	}

	// XXX reflog?

	return 0;
}

static int reftable_create_symref(struct ref_store *ref_store,
				  const char *refname, const char *target,
				  const char *logmsg)
{
	struct reftable_ref_store *refs = (struct reftable_ref_store*)ref_store;
	struct write_create_symref_arg arg = {
		.stack = refs->stack,
		.refname = refname,
		.target = target,
		.logmsg = logmsg
	};
	return stack_add(refs->stack, &write_create_symref_table, &arg);
}


struct write_rename_arg {
	struct stack *stack;
	const char *oldname;
	const char *newname;
	const char *logmsg;
};

static int write_rename_table(struct writer *writer, void *argv) {
	struct write_rename_arg *arg = (struct write_rename_arg*)argv;
	uint64_t ts = stack_next_update_index(arg->stack);
	struct ref_record ref = {};
	int err = stack_read_ref(arg->stack, arg->oldname, &ref);
	if (err) {
		goto exit;
	}

	// XXX should check that dest doesn't exist?
	free(ref.ref_name);
	ref.ref_name = strdup(arg->newname);
	writer_set_limits(writer, ts, ts);
	ref.update_index = ts;

	{
		struct ref_record todo[2]  = {};
		todo[0].ref_name =  (char *) arg->oldname;
		todo[0].update_index = ts;
		todo[1] = ref;
		todo[1].update_index = ts;

		err = writer_add_refs(writer, todo, 2);
		if (err < 0) {
			goto exit;
		}
	}

        if (ref.value != NULL) {
                struct log_record todo[2] = {};
                fill_log_record(&todo[0]);
                fill_log_record(&todo[1]);

                todo[0].ref_name = (char *) arg->oldname;
                todo[0].update_index = ts;
                todo[0].message = (char*)arg->logmsg;
                todo[0].old_hash = ref.value;
                todo[0].new_hash = NULL;

                todo[1].ref_name = (char *) arg->newname;
                todo[1].update_index = ts;
                todo[1].old_hash = NULL;
                todo[1].new_hash = ref.value;
                todo[1].message = (char*) arg->logmsg;

		err = writer_add_logs(writer, todo, 2);

                clear_log_record(&todo[0]);
                clear_log_record(&todo[1]);

		if (err < 0) {
			goto exit;
		}

        } else {
                // symrefs?
        }

exit:
	ref_record_clear(&ref);
	return err;
}


static int reftable_rename_ref(struct ref_store *ref_store,
			    const char *oldrefname, const char *newrefname,
			    const char *logmsg)
{
	struct reftable_ref_store *refs = (struct reftable_ref_store*) ref_store;
	struct write_rename_arg arg = {
		.stack = refs->stack,
		.oldname = oldrefname,
		.newname = newrefname,
		.logmsg = logmsg,
	};
	return stack_add(refs->stack, &write_rename_table, &arg);
}

static int reftable_copy_ref(struct ref_store *ref_store,
			   const char *oldrefname, const char *newrefname,
			   const char *logmsg)
{
	BUG("reftable reference store does not support copying references");
}

struct reftable_reflog_ref_iterator {
        struct ref_iterator base;
        struct iterator iter;
        struct log_record log;
        struct object_id oid;
        char *last_name;
};

static int reftable_reflog_ref_iterator_advance(struct ref_iterator *ref_iterator)
{
        struct reftable_reflog_ref_iterator *ri = (struct reftable_reflog_ref_iterator*) ref_iterator;

        while (1) {
                int err = iterator_next_log(ri->iter, &ri->log);
                if (err > 0) {
                        return ITER_DONE;
                }
                if (err < 0) {
                        return ITER_ERROR;
                }

                ri->base.refname = ri->log.ref_name;
                if (ri->last_name != NULL && 0 == strcmp(ri->log.ref_name, ri->last_name)) {
                        continue;
                }

                free(ri->last_name);
                ri->last_name = xstrdup(ri->log.ref_name);
                // XXX const?
                memcpy(&ri->oid.hash, ri->log.new_hash, GIT_SHA1_RAWSZ);
                return ITER_OK;
        }
}

static int reftable_reflog_ref_iterator_peel(struct ref_iterator *ref_iterator,
                                             struct object_id *peeled)
{
        BUG("not supported.");
	return -1;
}

static int reftable_reflog_ref_iterator_abort(struct ref_iterator *ref_iterator)
{
	struct reftable_reflog_ref_iterator *ri = (struct reftable_reflog_ref_iterator*) ref_iterator;
	log_record_clear(&ri->log);
	iterator_destroy(&ri->iter);
	return 0;
}

static struct ref_iterator_vtable reftable_reflog_ref_iterator_vtable = {
	reftable_reflog_ref_iterator_advance,
	reftable_reflog_ref_iterator_peel,
	reftable_reflog_ref_iterator_abort
};

static struct ref_iterator *reftable_reflog_iterator_begin(struct ref_store *ref_store)
{
        struct reftable_reflog_ref_iterator *ri = xcalloc(sizeof(*ri), 1);
        struct reftable_ref_store *refs = (struct reftable_ref_store*) ref_store;

	struct merged_table *mt = stack_merged_table(refs->stack);
        int err = merged_table_seek_log(mt, &ri->iter, "");
        if (err < 0) {
                free(ri);
                return NULL;
        }

	base_ref_iterator_init(&ri->base, &reftable_reflog_ref_iterator_vtable, 1);
	ri->base.oid = &ri->oid;

	return empty_ref_iterator_begin();
}

static int reftable_for_each_reflog_ent_newest_first(struct ref_store *ref_store,
                                        const char *refname,
                                        each_reflog_ent_fn fn, void *cb_data)
{
        struct iterator it = { };
        struct reftable_ref_store *refs = (struct reftable_ref_store*) ref_store;
        struct merged_table *mt = stack_merged_table(refs->stack);
        int err = merged_table_seek_log(mt, &it, refname);
        struct log_record log = {};

        while (err == 0)  {
                err = iterator_next_log(it, &log);
                if (err != 0) {
                        break;
                }

                if (0 != strcmp(log.ref_name, refname)) {
                        break;
                }

                {
                        struct object_id old_oid = {};
                        struct object_id new_oid = {};

                        memcpy(&old_oid.hash, log.old_hash, GIT_SHA1_RAWSZ);
                        memcpy(&new_oid.hash, log.new_hash, GIT_SHA1_RAWSZ);

                        // XXX committer = email? name?
                        if (fn(&old_oid, &new_oid, log.name, log.time, log.tz_offset, log.message, cb_data)) {
                                err = -1;
                                break;
                        }
                }
        }

        log_record_clear(&log);
        iterator_destroy(&it);
        if (err > 0) {
                err = 0;
        }
	return err;
}

static int reftable_for_each_reflog_ent_oldest_first(struct ref_store *ref_store,
                                                     const char *refname,
                                                     each_reflog_ent_fn fn,
                                                     void *cb_data)
{
        struct iterator it = { };
        struct reftable_ref_store *refs = (struct reftable_ref_store*) ref_store;
        struct merged_table *mt = stack_merged_table(refs->stack);
        int err = merged_table_seek_log(mt, &it, refname);

        struct log_record *logs = NULL;
        int cap = 0;
        int len = 0;

        printf("oldest first\n");
        while (err == 0)  {
                struct log_record log = {};
                err = iterator_next_log(it, &log);
                if (err != 0) {
                        break;
                }

                if (0 != strcmp(log.ref_name, refname)) {
                        break;
                }

                if (len == cap) {
                        cap  =2*cap + 1;
                        logs = realloc(logs, cap * sizeof(*logs));
                }

                logs[len++] = log;
        }

        for (int i = len; i--; ) {
                struct log_record *log = &logs[i];
                struct object_id old_oid = {};
                struct object_id new_oid = {};

                memcpy(&old_oid.hash, log->old_hash, GIT_SHA1_RAWSZ);
                memcpy(&new_oid.hash, log->new_hash, GIT_SHA1_RAWSZ);

                // XXX committer = email? name?
                if (!fn(&old_oid, &new_oid, log->name, log->time, log->tz_offset, log->message, cb_data)) {
                        err = -1;
                        break;
                }
        }

        for (int i = 0; i < len; i++) {
                log_record_clear(&logs[i]);
        }
        free(logs);

        iterator_destroy(&it);
        if (err > 0) {
                err = 0;
        }
	return err;
}

static int reftable_reflog_exists(struct ref_store *ref_store,
                                  const char *refname)
{
        // always exists.
	return 1;
}

static int reftable_create_reflog(struct ref_store *ref_store,
                                  const char *refname, int force_create,
                                  struct strbuf *err)
{
        return 0;
}

static int reftable_delete_reflog(struct ref_store *ref_store,
                                  const char *refname)
{
	return 0;
}

static int reftable_reflog_expire(struct ref_store *ref_store,
				const char *refname, const struct object_id *oid,
				unsigned int flags,
				reflog_expiry_prepare_fn prepare_fn,
				reflog_expiry_should_prune_fn should_prune_fn,
				reflog_expiry_cleanup_fn cleanup_fn,
				void *policy_cb_data)
{
        /*
          XXX

          This doesn't fit with the reftable API. If we are expiring for space
          reasons, the expiry should be combined with a compaction, and there
          should be a policy that can be called for all refnames, not for a
          single ref name.

          If this is for cleaning up individual entries, we'll have to write
          extra data to create tombstones.
         */
	return 0;
}


static int reftable_read_raw_ref(struct ref_store *ref_store,
				 const char *refname, struct object_id *oid,
				 struct strbuf *referent, unsigned int *type)
{
	struct reftable_ref_store *refs = (struct reftable_ref_store*) ref_store;
	struct ref_record ref = {};
	int err = stack_read_ref(refs->stack, refname, &ref);
	if (err) {
		goto exit;
	}
	if (ref.target != NULL) {
		strbuf_reset(referent);
		strbuf_addstr(referent, ref.target);
		*type |= REF_ISSYMREF;
 	} else  {
		memcpy(oid->hash, ref.value, GIT_SHA1_RAWSZ);
	}

exit:
	ref_record_clear(&ref);
	return err;
}

struct ref_storage_be refs_be_reftable = {
	&refs_be_files,
	"reftable",
	reftable_ref_store_create,
	reftable_init_db,
	reftable_transaction_prepare,
	reftable_transaction_finish,
	reftable_transaction_abort,
	reftable_transaction_commit,

	reftable_pack_refs,
	reftable_create_symref,
	reftable_delete_refs,
	reftable_rename_ref,
	reftable_copy_ref,

	reftable_ref_iterator_begin,
	reftable_read_raw_ref,

	reftable_reflog_iterator_begin,
	reftable_for_each_reflog_ent_newest_first,
	reftable_for_each_reflog_ent_oldest_first,
	reftable_reflog_exists,
	reftable_create_reflog,
	reftable_delete_reflog,
	reftable_reflog_expire
};
