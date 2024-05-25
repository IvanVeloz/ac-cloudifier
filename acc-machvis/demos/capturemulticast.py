#!/usr/bin/env python3

# capturemulticast.py
# Demonstrates opening an RTP multicast through Gstreamer

import cv2 as cv

gspipe = 'udpsrc port=5000 address=239.255.255.250 caps="application/x-rtp, encoding-name=JPEG" ! rtpjpegdepay ! jpegparse ! decodebin ! videoconvert ! appsink drop=1'
fakepipe = 'videotestsrc ! appsink'
cap = cv.VideoCapture(fakepipe, cv.CAP_GSTREAMER)


assert(cap.isOpened() == True)

ret, img = cap.read()
assert(ret == True)

cv.imshow("Video feed", img)

