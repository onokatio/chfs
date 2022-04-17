#include <sys/stat.h>
#include <stdlib.h>
#include <margo.h>
#include "ring_types.h"
#include "kv_types.h"
#include "fs_types.h"
#include "fs_rpc.h"
#include "log.h"

static int fs_rpc_timeout_msec;

static struct env {
	margo_instance_id mid;
	hg_id_t create_rpc, stat_rpc;
	hg_id_t write_rpc, read_rpc, write_rdma_rpc, read_rdma_rpc;
	hg_id_t copy_rdma_rpc;
	hg_id_t truncate_rpc, remove_rpc, readdir_rpc, unlink_all_rpc;
} env;

static hg_return_t
create_rpc_handle(const char *server, hg_id_t rpc_id, hg_handle_t *h,
	const char *diag)
{
	hg_addr_t addr;
	hg_return_t ret;

	ret = margo_addr_lookup(env.mid, server, &addr);
	if (ret != HG_SUCCESS) {
		log_error("%s (lookup): %s", diag, HG_Error_to_string(ret));
		return (ret);
	}
	ret = margo_create(env.mid, addr, rpc_id, h);
	if (ret != HG_SUCCESS)
		log_error("%s (create): %s", diag, HG_Error_to_string(ret));
	margo_addr_free(env.mid, addr);
	return (ret);
}

hg_return_t
fs_rpc_inode_create(const char *server, void *key, size_t key_size,
	uint32_t uid, uint32_t gid, mode_t mode, size_t chunk_size,
	const void *buf, size_t size, int *errp)
{
	hg_handle_t h;
	fs_create_in_t in;
	hg_return_t ret, ret2;
	static const char diag[] = "fs_rpc_inode_create";

	ret = create_rpc_handle(server, env.create_rpc, &h, diag);
	if (ret != HG_SUCCESS)
		return (ret);

	in.key.v = key;
	in.key.s = key_size;
	in.value.v = (void *)buf;
	in.value.s = size;
	in.uid = uid;
	in.gid = gid;
	in.mode = mode;
	in.chunk_size = chunk_size;
	ret = margo_forward_timed(h, &in, fs_rpc_timeout_msec);
	if (ret != HG_SUCCESS) {
		log_error("%s (forward): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	ret = margo_get_output(h, errp);
	if (ret != HG_SUCCESS) {
		log_error("%s (get_output): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	ret = margo_free_output(h, errp);
err:
	ret2 = margo_destroy(h);
	if (ret == HG_SUCCESS)
		ret = ret2;
	return (ret);
}

hg_return_t
fs_rpc_inode_stat(const char *server, void *key, size_t key_size,
	struct fs_stat *st, int *errp)
{
	hg_handle_t h;
	kv_byte_t in;
	fs_stat_out_t out;
	hg_return_t ret, ret2;
	static const char diag[] = "fs_rpc_inode_stat";

	ret = create_rpc_handle(server, env.stat_rpc, &h, diag);
	if (ret != HG_SUCCESS)
		return (ret);

	in.v = key;
	in.s = key_size;
	ret = margo_forward_timed(h, &in, fs_rpc_timeout_msec);
	if (ret != HG_SUCCESS) {
		log_error("%s (forward): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	ret = margo_get_output(h, &out);
	if (ret != HG_SUCCESS) {
		log_error("%s (get_output): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	*errp = out.err;
	if (out.err == 0)
		*st = out.st;

	ret = margo_free_output(h, &out);
err:
	ret2 = margo_destroy(h);
	if (ret == HG_SUCCESS)
		ret = ret2;
	return (ret);
}

hg_return_t
fs_rpc_inode_write(const char *server, void *key, size_t key_size,
	const void *buf, size_t *size, size_t offset, mode_t mode,
	size_t chunk_size, int *errp)
{
	hg_handle_t h;
	hg_return_t ret, ret2;
	fs_write_in_t in;
	kv_get_rdma_out_t out;
	static const char diag[] = "fs_rpc_inode_write";

	ret = create_rpc_handle(server, env.write_rpc, &h, diag);
	if (ret != HG_SUCCESS)
		return (ret);

	in.key.v = key;
	in.key.s = key_size;
	in.value.v = (void *)buf;
	in.value.s = *size;
	in.offset = offset;
	in.mode = mode;
	in.chunk_size = chunk_size;
	ret = margo_forward_timed(h, &in, fs_rpc_timeout_msec);
	if (ret != HG_SUCCESS) {
		log_error("%s (forward): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	ret = margo_get_output(h, &out);
	if (ret != HG_SUCCESS) {
		log_error("%s (get_output): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	*errp = out.err;
	if (out.err == 0 && *size > out.value_size)
		*size = out.value_size;
	ret = margo_free_output(h, &out);
err:
	ret2 = margo_destroy(h);
	if (ret == HG_SUCCESS)
		ret = ret2;
	return (ret);
}

hg_return_t
fs_rpc_inode_read(const char *server, void *key, size_t key_size, void *buf,
	size_t *size, size_t offset, int *errp)
{
	hg_handle_t h;
	hg_return_t ret, ret2;
	fs_read_in_t in;
	kv_get_out_t out;
	static const char diag[] = "fs_rpc_inode_read";

	ret = create_rpc_handle(server, env.read_rpc, &h, diag);
	if (ret != HG_SUCCESS)
		return (ret);

	in.key.v = key;
	in.key.s = key_size;
	in.size = *size;
	in.offset = offset;
	ret = margo_forward_timed(h, &in, fs_rpc_timeout_msec);
	if (ret != HG_SUCCESS) {
		log_error("%s (forward): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	ret = margo_get_output(h, &out);
	if (ret != HG_SUCCESS) {
		log_error("%s (get_output): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	*errp = out.err;
	if (out.err == 0) {
		if (*size > out.value.s)
			*size = out.value.s;
		memcpy(buf, out.value.v, *size);
	}
	ret = margo_free_output(h, &out);
err:
	ret2 = margo_destroy(h);
	if (ret == HG_SUCCESS)
		ret = ret2;
	return (ret);
}

hg_return_t
fs_rpc_inode_write_rdma_bulk(const char *server, void *key, size_t key_size,
	char *client, hg_bulk_t buf, hg_size_t *size, size_t offset,
	mode_t mode, size_t chunk_size, int *errp)
{
	hg_handle_t h;
	hg_return_t ret, ret2;
	fs_write_rdma_in_t in;
	kv_get_rdma_out_t out;
	static const char diag[] = "fs_rpc_inode_write_rdma_bulk";

	ret = create_rpc_handle(server, env.write_rdma_rpc, &h, diag);
	if (ret != HG_SUCCESS)
		return (ret);

	in.key.v = key;
	in.key.s = key_size;
	in.client = client;
	in.offset = offset;
	in.value = buf;
	in.value_size = *size;
	in.mode = mode;
	in.chunk_size = chunk_size;
	ret = margo_forward_timed(h, &in, fs_rpc_timeout_msec);
	if (ret != HG_SUCCESS) {
		log_error("%s (forward): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	ret = margo_get_output(h, &out);
	if (ret != HG_SUCCESS) {
		log_error("%s (get_output): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	*errp = out.err;
	if (out.err == 0)
		*size = out.value_size;
	ret = margo_free_output(h, &out);
err:
	ret2 = margo_destroy(h);
	if (ret == HG_SUCCESS)
		ret = ret2;
	return (ret);
}

hg_return_t
fs_rpc_inode_write_rdma(const char *server, void *key, size_t key_size,
	char *client, const void *buf, hg_size_t *size, size_t offset,
	mode_t mode, size_t chunk_size, int *errp)
{
	hg_bulk_t bulk;
	hg_return_t ret, ret2;
	void *b = (void *)buf;
	static const char diag[] = "fs_rpc_inode_write_rdma";

	if (*size == 0)
		return (HG_SUCCESS);
	ret = margo_bulk_create(env.mid, 1, &b, size, HG_BULK_READ_ONLY, &bulk);
	if (ret != HG_SUCCESS) {
		log_error("%s (bulk_create): %s", diag,
			HG_Error_to_string(ret));
		return (ret);
	}
	ret = fs_rpc_inode_write_rdma_bulk(server, key, key_size, client,
		bulk, size, offset, mode, chunk_size, errp);

	ret2 = margo_bulk_free(bulk);
	if (ret == HG_SUCCESS)
		ret = ret2;
	return (ret);
}

hg_return_t
fs_rpc_inode_read_rdma_bulk(const char *server, void *key, size_t key_size,
	char *client, hg_bulk_t buf, hg_size_t *size, size_t offset,
	int *errp)
{
	hg_handle_t h;
	hg_return_t ret, ret2;
	kv_put_rdma_in_t in;
	kv_get_rdma_out_t out;
	static const char diag[] = "fs_rpc_inode_read_rdma_bulk";

	ret = create_rpc_handle(server, env.read_rdma_rpc, &h, diag);
	if (ret != HG_SUCCESS)
		return (ret);

	in.key.v = key;
	in.key.s = key_size;
	in.client = client;
	in.offset = offset;
	in.value = buf;
	in.value_size = *size;
	ret = margo_forward_timed(h, &in, fs_rpc_timeout_msec);
	if (ret != HG_SUCCESS) {
		log_error("%s (forward): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	ret = margo_get_output(h, &out);
	if (ret != HG_SUCCESS) {
		log_error("%s (get_output): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	*errp = out.err;
	if (out.err == 0)
		*size = out.value_size;
	ret = margo_free_output(h, &out);
err:
	ret2 = margo_destroy(h);
	if (ret == HG_SUCCESS)
		ret = ret2;
	return (ret);
}

hg_return_t
fs_rpc_inode_read_rdma(const char *server, void *key, size_t key_size,
	char *client, void *buf, hg_size_t *size, size_t offset,
	int *errp)
{
	hg_bulk_t bulk;
	hg_return_t ret, ret2;
	static const char diag[] = "fs_rpc_inode_read_rdma";

	if (*size == 0)
		return (HG_SUCCESS);
	ret = margo_bulk_create(env.mid, 1, &buf, size,
		HG_BULK_WRITE_ONLY, &bulk);
	if (ret != HG_SUCCESS) {
		log_error("%s (bulk_create): %s", diag,
			HG_Error_to_string(ret));
		return (ret);
	}
	ret = fs_rpc_inode_read_rdma_bulk(server, key, key_size, client,
		bulk, size, offset, errp);

	ret2 = margo_bulk_free(bulk);
	if (ret == HG_SUCCESS)
		ret = ret2;
	return (ret);
}

hg_return_t
fs_rpc_inode_copy_rdma_bulk(const char *server, void *key, size_t key_size,
	char *client, struct fs_stat *st, hg_bulk_t buf, hg_size_t size,
	int *errp)
{
	hg_handle_t h;
	hg_return_t ret, ret2;
	fs_copy_rdma_in_t in;
	static const char diag[] = "fs_rpc_inode_copy_rdma_bulk";

	ret = create_rpc_handle(server, env.copy_rdma_rpc, &h, diag);
	if (ret != HG_SUCCESS)
		return (ret);

	in.key.v = key;
	in.key.s = key_size;
	in.client = client;
	in.stat = *st;
	in.value = buf;
	in.value_size = size;
	in.flag = 1;	/* do not forward */
	ret = margo_forward_timed(h, &in, fs_rpc_timeout_msec);
	if (ret != HG_SUCCESS) {
		log_error("%s (forward): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	ret = margo_get_output(h, errp);
	if (ret != HG_SUCCESS) {
		log_error("%s (get_output): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	ret = margo_free_output(h, errp);
err:
	ret2 = margo_destroy(h);
	if (ret == HG_SUCCESS)
		ret = ret2;
	return (ret);
}

hg_return_t
fs_rpc_inode_copy_rdma(const char *server, void *key, size_t key_size,
	char *client, struct fs_stat *st, void *buf, hg_size_t size, int *errp)
{
	hg_bulk_t bulk;
	hg_return_t ret, ret2;
	static const char diag[] = "fs_rpc_inode_copy_rdma";

	if (size == 0)
		return (HG_SUCCESS);
	ret = margo_bulk_create(env.mid, 1, &buf, &size, HG_BULK_READ_ONLY,
		&bulk);
	if (ret != HG_SUCCESS) {
		log_error("%s (bulk_create): %s", diag,
			HG_Error_to_string(ret));
		return (ret);
	}
	ret = fs_rpc_inode_copy_rdma_bulk(server, key, key_size, client, st,
		bulk, size, errp);

	ret2 = margo_bulk_free(bulk);
	if (ret == HG_SUCCESS)
		ret = ret2;
	return (ret);
}

hg_return_t
fs_rpc_inode_truncate(const char *server, void *key, size_t key_size,
	off_t len, int *errp)
{
	hg_handle_t h;
	hg_return_t ret, ret2;
	fs_truncate_in_t in;
	int32_t err;
	static const char diag[] = "fs_rpc_inode_truncate";

	ret = create_rpc_handle(server, env.truncate_rpc, &h, diag);
	if (ret != HG_SUCCESS)
		return (ret);

	in.key.v = key;
	in.key.s = key_size;
	in.len = len;
	ret = margo_forward_timed(h, &in, fs_rpc_timeout_msec);
	if (ret != HG_SUCCESS) {
		log_error("%s (forward): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	ret = margo_get_output(h, &err);
	if (ret != HG_SUCCESS) {
		log_error("%s (get_output): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	*errp = err;
	ret = margo_free_output(h, &err);
err:
	ret2 = margo_destroy(h);
	if (ret == HG_SUCCESS)
		ret = ret2;
	return (ret);
}

hg_return_t
fs_rpc_inode_remove(const char *server, void *key, size_t key_size, int *errp)
{
	hg_handle_t h;
	hg_return_t ret, ret2;
	kv_byte_t in;
	int32_t err;
	static const char diag[] = "fs_rpc_inode_remove";

	ret = create_rpc_handle(server, env.remove_rpc, &h, diag);
	if (ret != HG_SUCCESS)
		return (ret);

	in.v = key;
	in.s = key_size;
	ret = margo_forward_timed(h, &in, fs_rpc_timeout_msec);
	if (ret != HG_SUCCESS) {
		log_error("%s (forward): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	ret = margo_get_output(h, &err);
	if (ret != HG_SUCCESS) {
		log_error("%s (get_output): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	*errp = err;
	ret = margo_free_output(h, &err);
err:
	ret2 = margo_destroy(h);
	if (ret == HG_SUCCESS)
		ret = ret2;
	return (ret);
}

hg_return_t
fs_async_rpc_inode_unlink_chunk_all(const char *server, void *path,
	hg_handle_t *h)
{
	hg_return_t ret;
	static const char diag[] = "fs_rpc_inode_unlink_chunk_all";

	ret = create_rpc_handle(server, env.unlink_all_rpc, h, diag);
	if (ret != HG_SUCCESS)
		return (ret);

	ret = margo_forward_timed(*h, &path, fs_rpc_timeout_msec);
	if (ret != HG_SUCCESS)
		log_error("%s (forward): %s", diag, HG_Error_to_string(ret));
	return (ret);
}

hg_return_t
fs_async_rpc_inode_unlink_chunk_all_wait(hg_handle_t *h, int32_t *errp)
{
	hg_return_t ret, ret2;
	int32_t err;
	static const char diag[] = "fs_rpc_inode_unlink_chunk_all_wait";

	ret = margo_get_output(*h, &err);
	if (ret != HG_SUCCESS) {
		log_error("%s (get_output): %s", diag, HG_Error_to_string(ret));
		goto destroy;
	}
	*errp = err;
	ret = margo_free_output(*h, &err);
destroy:
	ret2 = margo_destroy(*h);
	if (ret == HG_SUCCESS)
		ret = ret2;
	return (ret);
}

static hg_return_t
fs_rpc_readdir_common(const char *server, const char *path, void *buf,
	int (*filler)(void *, const char *, const struct stat *, off_t),
	int with_replica, int *errp)
{
	hg_handle_t h;
	fs_readdir_out_t out;
	struct stat sb;
	hg_return_t ret, ret2;
	int i;
	static const char diag[] = "fs_rpc_readdir";

	ret = create_rpc_handle(server, env.readdir_rpc, &h, diag);
	if (ret != HG_SUCCESS)
		return (ret);

	ret = margo_forward_timed(h, &path, fs_rpc_timeout_msec);
	if (ret != HG_SUCCESS) {
		log_error("%s (forward): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	ret = margo_get_output(h, &out);
	if (ret != HG_SUCCESS) {
		log_error("%s (get_output): %s", diag, HG_Error_to_string(ret));
		goto err;
	}
	*errp = out.err;
	if (out.err == 0) {
		for (i = 0; i < out.n; ++i) {
			if (!with_replica && (out.fi[i].sb.mode & CHFS_S_IFREP))
				continue;
			memset(&sb, 0, sizeof(sb));
			sb.st_uid = out.fi[i].sb.uid;
			sb.st_gid = out.fi[i].sb.gid;
			sb.st_mode = out.fi[i].sb.mode;
			sb.st_size = out.fi[i].sb.size;
			sb.st_mtim = out.fi[i].sb.mtime;
			sb.st_ctim = out.fi[i].sb.ctime;
			if (filler(buf, out.fi[i].name, &sb, 0))
				break;
		}
	}
	ret = margo_free_output(h, &out);
err:
	ret2 = margo_destroy(h);
	if (ret == HG_SUCCESS)
		ret = ret2;
	return (ret);
}

hg_return_t
fs_rpc_readdir(const char *server, const char *path, void *buf,
	int (*filler)(void *, const char *, const struct stat *, off_t),
	int *errp)
{
	return (fs_rpc_readdir_common(server, path, buf, filler, 0, errp));
}

hg_return_t
fs_rpc_readdir_replica(const char *server, const char *path, void *buf,
	int (*filler)(void *, const char *, const struct stat *, off_t),
	int *errp)
{
	return (fs_rpc_readdir_common(server, path, buf, filler, 1, errp));
}

void
fs_client_init_internal(margo_instance_id mid, int timeout,
	hg_id_t create_rpc, hg_id_t stat_rpc, hg_id_t write_rpc,
	hg_id_t write_rdma_rpc, hg_id_t read_rpc, hg_id_t read_rdma_rpc,
	hg_id_t copy_rdma_rpc, hg_id_t truncate_rpc, hg_id_t remove_rpc,
	hg_id_t unlink_all_rpc)
{
	env.mid = mid;
	fs_rpc_timeout_msec = timeout;
	env.create_rpc = create_rpc;
	env.stat_rpc = stat_rpc;
	env.write_rpc = write_rpc;
	env.write_rdma_rpc = write_rdma_rpc;
	env.read_rpc = read_rpc;
	env.read_rdma_rpc = read_rdma_rpc;
	env.copy_rdma_rpc = copy_rdma_rpc;
	env.truncate_rpc = truncate_rpc;
	env.remove_rpc = remove_rpc;
	env.unlink_all_rpc = unlink_all_rpc;
}

void
fs_client_init_more_internal(hg_id_t read_rdma_rpc, hg_id_t readdir_rpc)
{
	if (read_rdma_rpc != -1)
		env.read_rdma_rpc = read_rdma_rpc;
	env.readdir_rpc = readdir_rpc;
}

void
fs_client_init(margo_instance_id mid, int timeout)
{
	env.mid = mid;
	fs_rpc_timeout_msec = timeout;
	env.create_rpc = MARGO_REGISTER(mid, "inode_create", fs_create_in_t,
		int32_t, NULL);
	env.stat_rpc = MARGO_REGISTER(mid, "inode_stat", kv_byte_t,
		fs_stat_out_t, NULL);
	env.write_rpc = MARGO_REGISTER(mid, "inode_write", fs_write_in_t,
		kv_get_rdma_out_t, NULL);
	env.read_rpc = MARGO_REGISTER(mid, "inode_read", fs_read_in_t,
		kv_get_out_t, NULL);
	env.write_rdma_rpc = MARGO_REGISTER(mid, "inode_write_rdma",
		fs_write_rdma_in_t, kv_get_rdma_out_t, NULL);
	env.read_rdma_rpc = MARGO_REGISTER(mid, "inode_read_rdma",
		kv_put_rdma_in_t, kv_get_rdma_out_t, NULL);
	env.copy_rdma_rpc = MARGO_REGISTER(mid, "inode_copy_rdma",
		fs_copy_rdma_in_t, int32_t, NULL);
	env.truncate_rpc = MARGO_REGISTER(mid, "inode_truncate",
		fs_truncate_in_t, int32_t, NULL);
	env.remove_rpc = MARGO_REGISTER(mid, "inode_remove", kv_byte_t,
		int32_t, NULL);
	env.readdir_rpc = MARGO_REGISTER(mid, "inode_readdir", hg_string_t,
		fs_readdir_out_t, NULL);
	env.unlink_all_rpc = MARGO_REGISTER(mid, "inode_unlink_chunk_all",
		hg_string_t, int32_t, NULL);
}

void
fs_client_term()
{
	margo_finalize(env.mid);
}
