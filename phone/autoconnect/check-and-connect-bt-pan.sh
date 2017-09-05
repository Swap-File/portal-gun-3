#!/bin/sh

# Edit this to match your MAc/PC/Other device that's providing the Bluetooth network access


SCRIPT=$(readlink -f $0)
SCRIPTPATH=`dirname $SCRIPT`

/sbin/ifconfig bnep0 > /dev/null 2>&1
status=$?
if [ $status -ne 0 ]; then
	echo "Connecting to $BT_MAC_ADDR"
fi


