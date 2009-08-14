#!/bin/sh

B=/proc/klife/boards/0/board

set -x

rmmod klife
insmod ~/klife.ko

echo test > /proc/klife/boards/create

cat > $B <<EOF
set 0 0
set 1 1
set 2 2
set 3 3
EOF

cat $B
