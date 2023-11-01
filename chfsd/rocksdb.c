#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "rocksdb/c.h"
#include "kv_err.h"
#include "kv.h"
#include "log.h"

static rocksdb_t *db = NULL;


void
kv_init(char *db_dir, char *engine, char *path, size_t size)
{
	static const char diag[] = "kv_init";

	if (db_dir == NULL || engine == NULL || path == NULL)
		log_fatal("%s: invalid argument", diag);

	long cpus = sysconf(_SC_NPROCESSORS_ONLN);

	rocksdb_options_t *options = rocksdb_options_create();
	rocksdb_options_increase_parallelism(
	    options, (int)(cpus));
	rocksdb_options_optimize_level_style_compaction(options, 0);
	rocksdb_options_set_create_if_missing(options, 1);

	char *p;
	struct stat sb;

	if ((p = malloc(strlen(db_dir) + 1 + strlen(path) + 1)) == NULL)
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
	log_debug("local rocksdb put: key=%s ", (char *)key);
	//for(int i=0; i<value_size; i++)
		//log_debug("local rocksdb put: value[%d]=%02hhX ", i, ((uint8_t *)value)[i]);
	char *err = NULL;
	rocksdb_put(db, rocksdb_writeoptions_create(), key, key_size, value,
		    value_size, &err);
	if(err==NULL){
		return KV_SUCCESS;
	} else {
		log_debug("local rocksdb put: error: %s", err);
		return KV_ERR_UNKNOWN;
	}
}

int
kv_put_addr(void *key, size_t key_size, void **value, size_t value_size)
{
	log_info("local rocksdb put addr: not implemented");
	return (KV_ERR_NOT_SUPPORTED);
}

int
kv_get(void *key, size_t key_size, void *value, size_t *value_size)
{
	log_debug("local rocksdb get: key=%s", (char *)key);
	char *err = NULL;
	value = rocksdb_get(db, rocksdb_readoptions_create(), key, key_size, value_size, &err);
	if(err!=NULL){
		log_debug("local rocksdb get: error: %s", err);
		return KV_ERR_UNKNOWN;
	}
	if (get_value == NULL) {
		log_debug("local rocksdb get: no entory");
		return KV_ERR_NO_ENTRY;
	}
	return KV_SUCCESS;
}

int
kv_update(void *key, size_t key_size,
    size_t off, void *value, size_t *value_size)
{
	log_debug("local rocksdb update: key=%s", (char *)key);
	char *err = NULL;

	char *get_value;
	size_t get_value_size;
	get_value = rocksdb_get(db, rocksdb_readoptions_create(), key, key_size, &get_value_size, &err);
	if (err != NULL){
		log_debug("local rocksdb update: error: %s", err);
		return KV_ERR_UNKNOWN;
	}
	if (get_value == NULL)
		return KV_ERR_NO_ENTRY;
	if (off > get_value_size)
		return KV_ERR_OUT_OF_RANGE;
	if (off + *value_size > get_value_size)
		off = get_value_size - *value_size;

	memcpy(get_value + off, value, *value_size);

	rocksdb_put(db, rocksdb_writeoptions_create(), key, key_size, get_value,
		    get_value_size, &err);
	if (err != NULL){
		log_debug("local rocksdb update: error: %s", err);
		return KV_ERR_UNKNOWN;
	}

	return KV_SUCCESS;
}

int
kv_pget(void *key, size_t key_size, size_t off, void *value, size_t *value_size)
{
	log_debug("local rocksdb pget: key=%s", (char *)key);
	char *err = NULL;
	size_t get_value_size;
	void *get_value = rocksdb_get(db, rocksdb_readoptions_create(), key,
				      key_size, &get_value_size, &err);
	if (err != NULL){
		log_debug("local rocksdb pget: error: %s", err);
		return KV_ERR_UNKNOWN;
	}
	if (get_value == NULL){
		log_debug("local rocksdb pget: no entory");
		return KV_ERR_NO_ENTRY;
	}
	if (off > get_value_size){
		log_debug("local rocksdb pget: out of range");
		return KV_ERR_OUT_OF_RANGE;
	}
	if (off + *value_size > get_value_size){
		*value_size = get_value_size - off;
	}
	memcpy(value, get_value + off, *value_size);
	log_debug("local rocksdb pget: success");
	return KV_SUCCESS;
}

int
kv_get_size(void *key, size_t key_size, size_t *value_size)
{
	log_debug("local rocksdb get_size: key=%s", (char *)key);
	char *err = NULL;
	rocksdb_get(db, rocksdb_readoptions_create(), key, key_size, value_size,
		    &err);
	if(err==NULL){
		return KV_SUCCESS;
	} else {
		log_debug("local rocksdb get_size: error: %s", err);
		return KV_ERR_UNKNOWN;
	}
}

int
kv_remove(void *key, size_t key_size)
{
	log_debug("local rocksdb remove: key=%s", (char *)key);
	void *hoge; //not use
	size_t *fuga; //not use
	char isexist = kv_get(key, key_size, hoge, fuga);
	if (isexist != KV_ERR_NO_ENTRY) {
		log_debug("local rocksdb remove isexist failed");
		return isexist;
	}

	char *err = NULL;
	rocksdb_delete(db, rocksdb_writeoptions_create(), key, key_size, &err);
	if(err==NULL){
		return KV_SUCCESS;
	} else {
		log_debug("local rocksdb remove: error: %s", err);
		return KV_ERR_UNKNOWN;
	}
}

int
kv_get_all_cb(int (*cb)(const char *, size_t, const char *, size_t, void *),
	void *arg)
{
	log_debug("local rocksdb get all cb");
	rocksdb_iterator_t *it = rocksdb_create_iterator(db, rocksdb_readoptions_create());
	rocksdb_iter_seek_to_first(it);
	for (; rocksdb_iter_valid(it); rocksdb_iter_next(it)) {
		size_t key_size, value_size;
		const char *key = rocksdb_iter_key(it, &key_size);
		const char *value = rocksdb_iter_value(it, &value_size);
		//log_debug("kv_get_all_cb: calling cb(key=%s, key_size=%lu, value=%s, value_size=%lu)", key, key_size, value, value_size);
		int ret = cb(key, key_size, value, value_size, arg);
		if (ret != 0){
			log_debug("kv_get_all_cb: stop!");
			rocksdb_iter_destroy(it);
			return KV_SUCCESS;
		}
	}
	rocksdb_iter_destroy(it);
	return KV_SUCCESS;
}
