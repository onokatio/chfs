% chfuse(1) CHFS
%
% December 18, 2020

# NAME
chfuse - mount CHFS

# SYNOPSIS
**chfuse** [_options_] <mount_point>

# DESCRIPTION
**chfuse** mounts a CHFS parallel consistent hashing file system.

# CHFUSE OPTIONS
\--server=CHFS_SERVER
: specifies a CHFS server.  The server can be specified by the CHFS_SERVER environment variable.

# ENVIRONMENT
CHFS_SERVER
: server addresses separated by ','

CHFS_CHUNK_SIZE
: chunk size.  Default is 64 KiB.

CHFS_ASYNC_ACCESS
: set 1 to enable asynchronous accesses.  Default is 0.

CHFS_BUF_SIZE
: buffer size.  Default is 0.

CHFS_LOOKUP_LOCAL
: set 1 to connect to a local chfsd only.

CHFS_RDMA_THRESH
: RDMA transfer is used when the size is larger than CHFS_RDMA_THRESH.  Zero means RDMA transfer is always used.  -1 means RDMA transfer is never used.  Default is 32 KiB.

CHFS_RPC_TIMEOUT_MSEC
: RPC timeout in milliseconds.  Zero means no timeout.  Default is 30000 milliseconds (30 seconds).

CHFS_NODE_LIST_CACHE_TIMEOUT
: node list cache timeout in seconds.  Zero means no timeout. Default is 120 seconds.

CHFS_LOG_PRIORITY:
: maximum log priority to report.  Default is notice.
