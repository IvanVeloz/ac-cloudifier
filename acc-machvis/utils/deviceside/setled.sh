#!/bin/sh

LED_RED=23
LED_BLUE=22
LED_IR=5

LED_RED_ON=0
LED_BLUE_ON=0
LED_IR_ON=1

LED_RED_OFF=1
LED_BLUE_OFF=1
LED_IR_OFF=0

echousage()
{
    echo "Usage: $0 COLOR STATE" 
    echo ""
    echo "List of colors:"
    echo "red       Red status LED"
    echo "blue      Blue status LED"
    echo "ir        Infrared flood light"
    echo ""
    echo "List of states:"
    echo "on        Turns on"
    echo "off       Turns off"
}

if [ -z "$1" ] || [ -z "$2" ]; then
    echousage
elif [ "$1" = "red" ]; then
    if [ "$2" = "on" ]; then
        pigs w $LED_RED $LED_RED_ON
    elif [ "$2" = "off" ]; then
        pigs w $LED_RED $LED_RED_OFF
    else
        echousage
    fi
elif [ "$1" = "blue" ]; then
    if [ "$2" = "on" ]; then
        pigs w $LED_BLUE $LED_BLUE_ON
    elif [ "$2" = "off" ]; then
        pigs w $LED_BLUE $LED_BLUE_OFF
    else
        echousage
    fi
elif [ "$1" = "ir" ]; then
    if [ "$2" = "on" ]; then
        pigs w $LED_IR $LED_IR_ON
        pigs w 18 $LED_IR_ON
    elif [ "$2" = "off" ]; then
        pigs w $LED_IR $LED_IR_OFF
        pigs w 18 $LED_IR_ON
    else
        echousage
    fi
else
    echousage
fi


