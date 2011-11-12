#!/bin/sh
#
# modified by Fabian Bieker <fabian.bieker@web.de>
#
# this is just a dummy, more to come

. scripts/misc/base.sh

SRCIP=$1
SRCPORT=$2
DSTIP=$3
DSTPORT=$4

SERVICE="rpc"
HOST="serv"

my_start

while read name; do

	# remove control-characters
	name=`echo $name | sed s/[[:cntrl:]]//g`

	echo "$name" >> $LOG
done
my_stop
