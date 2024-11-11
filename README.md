# CHFS/Cache - Parallel caching file system for node-local storages

CHFS is a parallel consistent hashing file system created instantly using node-local storages such as persistent memory and NVMe SSD.  It exploits the performance of persistent memory using persistent in-memory key-value store pmemkv.  For NVMe SSD, it uses the POSIX backend.  It supports RDMA high performance data access.

CHFS/Cache provides a caching mechanism against a backend parallel file system.  Files in the backend parallel file system are automatically cached on demand or manually staged-in.  Output files are automatically flushed by I/O-aware flushing to maximize the performance of CHFS/Cache.

## Quick installation steps

1. Install development kits and required tools

       # apt install gcc g++ automake cmake libtool pkgconf libssl-dev libfuse-dev fuse
       # apt install git sudo vim curl wget pandoc gdb numactl

1. (Optional) Install pmamkv for a pmemkv backend

       # apt install libpmemkv-dev libmemkind-dev libtbb-dev rapidjson-dev

   For details, see https://pmem.io/pmemkv/

1. Install libfabric, mercury, argobots and mochi-margo

   - See [Docker file](dev/Dockerfile)

1. Install CHFS

       % git clone https://github.com/otatebe/chfs.git
       % cd chfs
       % autoreconf -i
       % ./configure [--prefix=PREFIX] [--with-pmemkv] [--enable-zero-copy-read-rdma]
       % make
       # make install

   If --with-pmemkv is not specified, CHFS uses a POSIX backend.


## Quick installation steps using Spack

1. Install development kits

       # apt install git python3 gcc automake libtool cmake pkgconf vim libfuse-dev

(Optional) Install pandoc to generate manual pages

       # apt install pandoc

1. Install Spack

       % git clone -c feature.manyFiles=true --depth 1 https://github.com/spack/spack.git
       % . spack/share/spack/setup-env.sh

   For details, see https://spack.readthedocs.io/
 
1. Install dependencies (mochi-margo)

       % git clone https://github.com/otatebe/chfs.git
       % cd chfs
       % spack env create -d .
       % spack env activate -d .

   For details, see https://mochi.readthedocs.io/

1. (Optional) Install pmemkv for a pmemkv backend

       # apt install libpmemkv-dev libmemkind-dev libtbb-dev rapidjson-dev

   For details, see https://pmem.io/pmemkv/

1. Install CHFS

       % autoreconf -i
       % ./configure [--prefix=PREFIX] [--with-pmemkv] [--enable-zero-copy-read-rdma]
       % make
       # make install

   If --with-pmemkv is not specified, CHFS uses a POSIX backend.  To use chfs, `spack env activate` is required.

## Quick installation steps by Spack

1. Install required packages

       # apt install git python3 gcc automake libtool cmake pkgconf vim libfuse-dev fuse libpmemkv-dev libmemkind-dev libtbb-dev rapidjson-dev libopenmpi-dev

1. Install Spack

       % git clone -c feature.manyFiles=true --depth 1 https://github.com/spack/spack.git
       % . spack/share/spack/setup-env.sh

   For details, see https://spack.readthedocs.io/

1. Install CHFS

       % git clone https://github.com/range3/chfs-spack-packages.git
       % spack repo add chfs-spack-packages
       % spack external find automake autoconf libtool cmake m4 pkgconf libfuse
       % spack config edit packages
       manually add pmemkv
       % spack spec chfs@master ^mercury~boostsys
       see what packages will be built
       % spack install chfs@master ^mercury~boostsys

   To enable verbs, see above.  To use chfs, `spack load chfs` is required.

## How to create file system

1. Create CHFS

       % eval `chfsctl [-h hostfile] [-p verbs] [-D] [-c /dev/dax0.0] [-b /back/end/path] [-m /mount/point] start`

   This executes chfsd servers and mounts the CHFS at /mount/point on hosts specified by the hostfile.  The -p option specifies a communication protocol.  The -c option specifies a devdax device or a scratch directory on each host.  For the devdax device, -D option is required.

   The backend directory typically in a parallel file system can be specified by the -b option.  Files in the backend directory can be transparently accessed at the CHFS mount directory.  For efficient access, files can be staged-in by `chstagein` command beforehand.  This is an example to stage-in all files in the backend directory.

       % cd /back/end/path
       % find . | xargs [ mpirun ... ] chstagein

   `chstagein` can be executed with and without mpirun.  The output files will be flushed automatically to the backend directory.  It is possible to ensure flushing all dirty files by `chfs_sync()` or `chfsctl stop`.

   A pmem obj pool should be created with the layout pmemkv by `pmempool create -l pmemkv obj /dev/dax0.0`.  For user-level access, the permission of the device should be modified; bad block check should be disabled by `pmempool feature --disable CHECK_BAD_BLOCKS /dev/dax0.0`.

   chfsctl outputs the setting of CHFS_SERVER, CHFS_BACKEND_PATH, and CHFS_SUBDIR_PATH environment variables, which are used to execute chfuse and CHFS commands.

   For details, see [manual page of chfsctl](doc/chfsctl.1.md).

## How to use

1. Mount the CHFS

   CHFS is mounted by the chfsctl command.  If you need to mount it on other hosts, chfuse command is used;

       % chfuse <mount_point>

   CHFS_SERVER and other environment variables, which are the output of chfsctl command, should be defined.

   For details, see [manual page of chfuse](doc/chfuse.1.md).

## CHFS commands

- [chlist(1)](doc/chlist.1.md) - list CHFS servers
- [chmkdir(1)](doc/chmkdir.1.md) - create a directory in CHFS
- [chrmdir(1)](doc/chrmdir.1.md) - remove a directory in CHFS
- [chfind(1)](doc/chfind.1.md) - search for files in CHFS
- [chstagein(1)](doc/chstagein.1.md) - stage-in files to CHFS

## Environment variable

- CHFS_SERVER - server addresses separated by ','
- CHFS_BACKEND_PATH - backend path
- CHFS_CHUNK_SIZE - chunk size
- CHFS_ASYNC_ACCESS - set 1 to enable asynchronous accesses
- CHFS_BUF_SIZE - buffer size
- CHFS_LOOKUP_LOCAL - set 1 to connect to a local chfsd only
- CHFS_LOOKUP_RELAY_GROUP - group number for relayed connections to chfsd
- CHFS_LOOKUP_RELAY_GROUP_AUTO - set 1 for setting group number automatically for relayed connections to chfsd
- CHFS_RDMA_THRESH - RDMA transfer is used when the size is larger than CHFS_RDMA_THRESH
- CHFS_RPC_TIMEOUT_MSEC - RPC timeout in milliseconds
- CHFS_NODE_LIST_CACHE_TIMEOUT - node list cache timeout in seconds
- CHFS_LOG_PRIORITY - maximum log priority to report

When you use pmemkv, devdax is desirable.  When you use fsdax, the following environment variable is recommended to touch every page of the persistent memory pool, while the start up time of chfsd becomes slow.

- PMEMOBJ_CONF="prefault.at_open=1;prefault.at_create=1"

## Open MPI with CHFS ADIO
ROMIO ADIO for CHFS is available.  With the ROMIO ADIO for CHFS, MPI-IO applications can access CHFS without any source code modification.  you can access CHFS by chfs:/path/name.

1. Installation

       % apt install gfortran bzip2 flex libpmix-dev libnl-3-dev libibverbs-dev
       % wget https://download.open-mpi.org/release/open-mpi/v4.1/openmpi-4.1.6.tar.bz2
       % tar xfp openmpi-4.1.6.tar.bz2
       % cd openmpi-4.1.6
       % wget https://raw.githubusercontent.com/otatebe/chfs/cache/dev/ompi/ad_chfs.patch
       % patch -p1 < ad_chfs.patch
       % (cd ompi/mca/io/romio321/romio/ && ./autogen.sh)
       % mkdir build && cd build
       % ../configure --enable-mpirun-prefix-by-default --with-pmix=/usr/lib/x86_64-linux-gnu/pmix2
       % make -j $(nproc)
       # make install

1. install CHFS again with MPI for parallel find and parallel stage-in

       % cd chfs
       % ./configure [--prefix=PREFIX] [--with-pmemkv] [--enable-zero-copy-read-rdma]
       % make
       # make install

1. How to use

       % mpirun --mca io romio321 -x CHFS_SERVER -x CHFS_BACKEND_PATH -x CHFS_SUBDIR_PATH ...

## IOR and mdtest

1. Installation

       % git clone https://github.com/hpc/ior.git
       % cd ior
       % ./bootstrap
       % ./configure [--prefix=PREFIX]
       % make
       # make install

1. How to use

       % mpirun -x CHFS_SERVER -x CHFS_BACKEND_PATH -x CHFS_SUBDIR_PATH ior -a CHFS [--chfs.chunk_size=SIZE]

   Large chunk size, i.e. 1 MiB, should be specified for best performance.  If you are using Open MPI with CHFS ADIO, it is possible to use the MPIIO backend by `-a MPIIO` with `chfs:/path/name`.

## CHFS API

The following APIs are supported.

    int chfs_init(const char *server);
    int chfs_initialized();
    int chfs_term();
    int chfs_term_without_sync();
    int chfs_size();
    const char *chfs_version();
    void chfs_set_chunk_size(int chunk_size);
    void chfs_set_async_access(int enable);
    void chfs_set_buf_size(int buf_size);
    void chfs_set_stagein_buf_size(int buf_size);
    void chfs_set_rdma_thresh(size_t thresh);
    void chfs_set_rpc_timeout_msec(int timeout_msec);
    void chfs_set_node_list_cache_timeout(int timeout_sec);

    int chfs_create(const char *path, int32_t flags, mode_t mode);
    int chfs_create_chunk_size(const char *path, int32_t flags, mode_t mode,
            int chunk_size);
    int chfs_open(const char *path, int32_t flags);
    int chfs_close(int fd);
    ssize_t chfs_pwrite(int fd, const void *buf, size_t size, off_t offset);
    ssize_t chfs_write(int fd, const void *buf, size_t size);
    ssize_t chfs_pread(int fd, void *buf, size_t size, off_t offset);
    ssize_t chfs_read(int fd, void *buf, size_t size);
    off_t chfs_seek(int fd, off_t off, int whence);
    int chfs_fsync(int fd);
    int chfs_truncate(const char *path, off_t len);
    int chfs_ftruncate(int fd, off_t len);
    int chfs_unlink(const char *path);
    int chfs_mkdir(const char *path, mode_t mode);
    int chfs_rmdir(const char *path);
    int chfs_stat(const char *path, struct stat *st);
    int chfs_fstat(int fd, struct stat *st);
    int chfs_access(const char *path, int mode);
    int chfs_readdir(const char *path, void *buf,
            int (*filler)(void *, const char *, const struct stat *, off_t));
    int chfs_readdir_index(const char *path, int index, void *buf,
            int (*filler)(void *, const char *, const struct stat *, off_t));
    int chfs_symlink(const char *target, const char *path);
    int chfs_readlink(const char *path, char *buf, size_t size);
    void chfs_sync();
    int chfs_stagein(const char *path);

## References

1. Osamu Tatebe, Kazuki Obata, Kohei Hiraga, Hiroki Ohtsuji, "[CHFS: Parallel Consistent Hashing File System for Node-local Persistent Memory](https://doi.org/10.1145/3492805.3492807)", Proceedings of the ACM International Conference on High Performance Computing in Asia-Pacific Region (HPC Asia 2022), pp.115-124, 2022

1. Osamu Tatebe, Hiroki Ohtsuji, "[Caching Support for CHFS Node-local Persistent Memory File System](https://doi.org/10.1109/IPDPSW55747.2022.00182)", Proceedings of 3rd Workshop on Extreme-Scale Storage and Analysis (ESSA 2022), pp.1103-1110, 2022

1. Osamu Tatebe, Kohei Hiraga, Hiroki Ohtsuji, "[I/O-Aware Flushing for HPC Caching Filesystem](https://doi.org/10.1109/CLUSTERWorkshops61457.2023.00012)", Proceedings of 3rd Workshop on Re-envisioning Extreme-Scale I/O for Emerging Hybrid HPC Workloads (REX-IO), pp.11-17, 2023
