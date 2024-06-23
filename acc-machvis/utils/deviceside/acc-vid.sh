#!/bin/sh

if [ -z $1 ]; then
    MULTICAST_ADDR=127.0.0.1
else
    MULTICAST_ADDR=$1
fi

if [ -z $2 ]; then
    MULTICAST_PORT=5000
else
    MULTICAST_PORT=$2
fi

libcamera-vid -t 0 --nopreview --framerate 5 --codec h264 --width 1920 --height 1080 --inline --listen -o udp://$MULTICAST_ADDR:$MULTICAST_PORT
