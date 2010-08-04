#!/bin/sh
# Created by Exaos Lee, Mar. 31, 2010
#
### BEGIN INIT INFO
# Provides:          a2818
# Required-Start:    $syslog $remote_fs
# X-UnitedLinux-Should-Start: $time ypbind sendmail
# Required-Stop:     $syslog $remote_fs
# X-UnitedLinux-Should-Stop: $time ypbind sendmail
# Default-Start:     3 5
# Default-Stop:      0 1 2 6
# Short-Description: A2818 Interface
# Description:       Start CAEN-A2818 driver
### END INIT INFO
#

mod_name="a2818"
mod_file="/opt/DAQ/lib/kmods/${mod_name}.ko"
device="a2818"
group="daq"
mode="660"

dev_minors="`seq 0 7` `seq 16 23` `seq 32 39`"

. /lib/lsb/init-functions

# load module
function load_module {
    mod_list=`grep -e ${mod_name} /proc/devices`
    if [ -z "${mod_list}" ]; then
	if [ -f ${mod_file} ]; then
	    /sbin/insmod ${mod_file} $* || return 1
	else
	    /sbin/modprobe ${mod_name} $* || return 1
	fi
	echo "DONE!"
    fi
}

# remove devices
function remove_devices {
    rm -f /dev/${device}_*
}

# create device
function create_devices {
    load_module || return 1

    remove_devices

    echo "Creating device files ..."
    major=`cat /proc/devices | awk "\\$2==\"$device\" {print \\$1}"`
    for minor in ${dev_minors} ; do
	/bin/mknod /dev/${device}_$minor c $major $minor
    done
    chown root   /dev/${device}_*
    chgrp $group /dev/${device}_*
    chmod $mode  /dev/${device}_*
}

#############################################

function a2818_start {
    log_daemon_msg "Loading CAEN A2818 interface module" "a2818"

    create_devices

    log_end_msg $?
    return $?
}

function a2818_stop {
    log_daemon_msg "Removing CAEN A2818 interface module" "a2818"

    if `lsmod | grep -q ${mod_name}` ; then
	if [ -f ${mod_file} ]; then
	    rmmod ${mod_name}
	else
	    modprobe -r ${mod_name}
	fi
    else
	echo "Module ${mod_name} is not loaded."
    fi
    remove_devices

    log_end_msg $?
    return $?
}


function a2818_status {
    echo -n "Module ${mod_name} "
    if `lsmod | grep -q ${mod_name}` ; then
	echo "is loaded."	
    else
	echo "is not loaded."
    fi

    ls -l /dev/${device}_*
}

case "$1" in
    start)
	a2818_start
	;;
    stop)
	a2818_stop
	;;
    restart)
	a2818_stop && sleep 3
	a2818_start
	;;
    status)
	a2818_status
	;;
    *)
	echo "Usage: $0 {start|stop|status|restart}"
	;;
esac

exit 0


