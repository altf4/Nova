#!/bin/sh
#
# $1: srcip, $2: srcport, $3: dstip, $4: dstport
#
# modified by Fabian Bieker <fabian.bieker@web.de>
# added protocol mismatch stuff
#

. scripts/misc/base.sh

SRCIP=$1
SRCPORT=$2
DSTIP=$3
DSTPORT=$4

SERVICE="ssh"
HOST="bps-pc10"


my_start

echo "SSH-1.99-OpenSSH_3.0.2p1"

while read name; do
	echo "$name" >> $LOG
	LINE=`echo "$name" | egrep -i "[\n ]"`
	if [ -z "$LINE" ]; then
		echo "Protocol mismatch."
		my_stop
	else
        echo "$name"
	fi
done
my_stop
