#!/bin/bash

# $Id: eathena.sh,v 1.5 2006/03/10 21:41:09 Platyna Exp $
#----------------------------------------------------------------------
# Description: Simple script to control eAthena locally.
# Author: Zuzanna K. Filutowska <platyna@platinum.linux.pl>
# Created at: Fri Feb 17 18:23:56 CET 2006
# License: GPL
# Copyright (c) 2006 Zuzanna K. Filutowska  All rights reserved.
#
#----------------------------------------------------------------------
# Configure section:
PATH=$PATH:.
SRVHOMEDIR=$HOME/tmwserver
#----------------------------------------------------------------------
# main()

cd ${SRVHOMEDIR}

eathena_start() {
    if [ -x ${SRVHOMEDIR}/eathena-monitor ]; 
	then echo "Starting eathena monitor..."
	     ${SRVHOMEDIR}/eathena-monitor ${SRVHOMEDIR}/conf/eathena-monitor.conf
	else echo "Eathena monitor binary is not executable or not found."
    fi
}

eathena_stop() {
    echo "Shutting down eathena monitor..."
    killall eathena-monitor
}

eathena_restart() {
    eathena_stop
    echo "Waiting for all eathena processes to end..."
    sleep 1
    eathena_start
}

case "$1" in
'start')
  eathena_start
  ;;
'stop')
  eathena_stop
  ;;
'restart')
  eathena_restart
  ;;
*)
  echo "usage $0 start|stop|restart"
esac
