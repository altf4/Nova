#!/bin/sh
# Test script for Honeyd
DATE=`date`
LOGDIR=/var/log/honeypot/
[ ! -e "$LOGDIR" ] && LOGDIR=/tmp
LOGFILE=$LOGDIR/log_test
echo "$DATE: Started From $1 Port $2" >> $LOGFILE
echo SSH-1.5-2.40
while read name
do
	echo "$name" >> $LOGFILE
        echo "$name"
done
