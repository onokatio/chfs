#!/bin/sh

set -eux

LANG=C
MDIR=/tmp/a

# clean up
chfsctl -h hosts -m $MDIR stop 2> /dev/null
chfsctl -h hosts clean

eval $(chfsctl -h hosts -m $MDIR -L log start)
chlist
cp ~/chfs/configure $MDIR
diff ~/chfs/configure $MDIR/configure
s1=$(wc ~/chfs/configure | awk '{ print $3 }')
s2=$(wc $MDIR/configure | awk '{ print $3 }')
[ $s1 = $s2 ]
s1=$(ls -l ~/chfs/configure | awk '{ print $5 }')
s2=$(ls -l $MDIR/configure | awk '{ print $5 }')
[ $s1 = $s2 ]
chfind $MDIR
mpirun -x PATH -x CHFS_SERVER -np 4 -hostfile hosts -map-by node ior -a CHFS -o $MDIR/test -g -w -r -R -G 12345
chfsctl -h hosts -m $MDIR stop

echo OK
