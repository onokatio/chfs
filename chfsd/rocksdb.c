#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "rocksdb/c.h"
#include "kv_err.h"
#include "kv.h"
#include "log.h"

static rocksdb_t *db = NULL;

static int
kv_err(int *rocckdb_err)
{
	switch (rocksdb_err) {
	case NULL:
		return (KV_SUCCESS);
	default:
		return (KV_ERR_UNKNOWN);
	}
}

void
kv_init(char *db_dir, char *engine, char *path, size_t size)
{
	if (db_dir == NULL || engine == NULL || path == NULL)
		log_fatal("%s: invalid argument", diag);

	rocksdb_options_t *options = rocksdb_options_create();
	rocksdb_options_increase_parallelism(
	    options, (int)(sysconf(_SC_NPROCESSORS_ONLN);));
	rocksdb_options_optimize_level_style_compaction(options, 0);
	rocksdb_options_set_create_if_missing(options, 1);

	char *p;
	int r;
	struct stat sb;
	static const char diag[] = "kv_init";

	if ((p = malloc(strlen(db_dir) + 1 + strlen(path) + 1) == NULL)
		log_fatal("%s: no memory", diag);

	if (stat(db_dir, &sb))
		log_fatal("%s: %s: %s", diag, db_dir, strerror(errno));

	if (S_ISDIR(sb.st_mode))
		sprintf(p, "%s/%s", db_dir, path);
	else
		sprintf(p, "%s", db_dir);
	log_info("kv_init: engine %s, path %s, size %ld", engine, p, size);

	char *err = NULL;
	db = rocksdb_open(options, p, &err);
	if(err != NULL || db == NULL)
		log_fatal("%s: %s", diag, err);

	free(p);
}

void
kv_term()
{
	rocksdb_close(db);
}

int
kv_put(void *key, size_t key_size, void *value, size_t value_size)
{
	log_debug("local rocksdb put: key=%s", (char *)key);
	char *err = NULL;
	rocksdb_put(db, rocksdb_writeoptions_create(), key, strlen(key), value,
		    strlen(value) + 1, &err);
	return kv_err(err);
}

int
kv_put_addr(void *key, size_t key_size, void **value, size_t value_size)
{
	log_info("local pmem put addr: not implemented");
	return (KV_ERR_NOT_SUPPORTED);
}

int
kv_get(void *key, size_t key_size, void *value, size_t *value_size)
{
	log_debug("local pmem get: key=%s", (char *)key);
	char *value =
	    rocksdb_get(db, rocksdb_readoptions_create(), key, strlen(key), &value_size, &err);
	return (kv_err(err));
}

int
kv_update(void *key, size_t key_size,
    size_t off, void *value, size_t *value_size)
{
	int err;
	int r;

	char *get_value;
	size_t get_value_size;
	get_value = rocksdb_get(db, rocksdb_readoptions_create(), key, key_size, &get_value_size, &err);
	if (err != NULL)
		return KV_ERR_UNKNOWN;
	if (get_value == NULL)
		return KV_ERR_NO_ENTRY;
	if (off > get_value_size)
		return KV_ERR_OUT_OF_RANGE;
	if (off + *value_size > get_value_size)
		off = get_value_size - *value_size;

	memcopy(get_value + off, value, *value_size);

	rocksdb_put(db, rocksdb_writeoptions_create(), key, key_size, get_value,
		    get_value_size, &err);
	if (err != NULL)
		return KV_ERR_UNKNOWN;

	return KV_SUCCESS;
}

int
kv_pget(void *key, size_t key_size, size_t off, void *value, size_t *value_size)
{
	void *value;
	get_value = rocksdb_get(db, rocksdb_readoptions_create(), key, key_size,
				value_size, &err);
	if (err != NULL)
		return KV_ERR_UNKNOWN;
	if (get_value == NULL)
		return KV_ERR_NO_ENTRY;
	if (off > get_value_size)
		return KV_ERR_OUT_OF_RANGE;
	if (off + *value_size > get_value_size)
		*value_size = get_value_size - off;
	memcopy(value, get_value + off, value_size);
	return KV_SUCCESS;
}

int
kv_get_size(void *key, size_t key_size, size_t *value_size)
{
	int err;
	rocksdb_get(db, rocksdb_readoptions_create(), key, key_size, value_size,
		    &err);
	return kv_err(err);
}

int
kv_remove(void *key, size_t key_size)
{
	rocksdb_delete(db, rocksdb_writeoptions_create(), key, key_size, &err);
	return kv_err(err);
}

int
kv_get_all_cb(int (*cb)(const char *, size_t, const char *, size_t, void *),
	void *arg)
{
	log_debug("local pmem get all cb");
	rocksdb_iterator_t *it = rocksdb_create_iterator(db, rocksdb_readoptions_create());
	for (; rocksdb_iter_valid(it); rocksdb_iter_next(it)){
		size_t key_size, value_size;
		char *key = rocksdb_iter_key(it, &key_size);
		char *value = rocksdb_iter_value(it, &value_size);
		int ret = cb(key, key_size, value, value_size, arg);
		if (ret != 0)
			break;
	}
}
