#!/bin/bash

SSHURI=root@ac-cloudifier.local
TSTAMP=$(date +%Y%m%dT%H%M%S)

if [ -z "$1" ]; then
	SAVEPATH=./
elif [[ "$1" == "--help" || "$1" == -* ]]; then
	echo "Usage: $0 [destination]"
	echo "getphoto gets the test.jpg photo from opencvdemo, "
	echo "located in root's home directory."
	exit 0
else
	SAVEPATH=$(dirname $1)
fi

ssh ${SSHURI} opencvdemo
scp ${SSHURI}:~/test.jpg ${SAVEPATH}/ac-cloudifier-${TSTAMP}.jpg
xdg-open ${SAVEPATH}/ac-cloudifier-${TSTAMP}.jpg

