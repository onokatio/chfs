% chfsd(8) CHFS
%
% December 18, 2020

# NAME
chfsd - CHFS server

# SYNOPSIS
**chfsd** [_options_] [_server_]

# DESCRIPTION
**chfsd** is a file server for CHFS parallel consistent hashing file system.  When _server_ is specified, it joins the server pool.  The server address is written to a server info file specified by the -S option.

# OPTIONS
-c db_dir
: specifies a database directory or a DAX device.  If the directory does not exist, it will be created.

-d
: enables debug mode.  In the debug mode, the server is executed in the foreground.

-f
: executes in the foreground.

-s db_size
: specifies a database size.  This option is only effective when using the pmemkv and fsdax.  Default is 1 GiB.

-p protocol
: specifies a protocol like sm, sockets, tcp, and verbs.  Default is sockets.

-h host
: specify hostname, IP address or interface name and the port number.  Before the port number, ':' is required.

-b backend_dir
: specify a backend directory to flush.

-B subdir
: specify a subdir that is considered to be a root directory in CHFS for the backend directory.  This option is effective with -b option.

-F num_flush_threads
: specify the number of flush threads.  Default is 1.  0 means no flush.  This option is effective with -b option.

-U flush_interval
: specify the flush interval in seconds.  If it is negative, dirty files are flushed immediately.  If it is zero or larger, dirty files are flushed after the interval from the last I/O activity.  Default is 1.  This option is effective with -b option.

-l log_file
: specifies a log file.

-P pid_file
: specifies a pid file.

-n vname
: specifies a vname which is used to construct a virtual name such as server_address:vname.

-N virtual_name
: specifies a virtual name.

-S server_info_file
: specifies a server info file where the server address will be written.

-t RPC_timeout_msec
: specifies a timeout for RPC in milliseconds.  Zero means no timeout.  Default is 300000 milliseconds (30 seconds).

-T nthreads
: specifies the number of threads of the chfsd.  Default is 4.

-I niothreads
: specifies the number of I/O threads of the chfsd.  This option is effective with ABT-IO.  Default is 2.

-H heartbeat_interval
: specifies the interval of heartbeat in second.  Default is 60 seconds.

-L log_priority
: specifies the maximum log priority to report.  Default is notice.

-Z
: enables profiling
