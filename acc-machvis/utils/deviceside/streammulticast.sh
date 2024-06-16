#!/bin/sh

# To be run on the embedded device (or device containing camera)

v4l2-ctl --set-parm=30
v4l2-ctl --set-fmt-video=width=640,height=480,pixelformat=JPEG --stream-mmap --stream-count=-1 --stream-to=- 2>/dev/null | \
gst-launch-1.0 fdsrc ! jpegparse ! rtpjpegpay ! udpsink host=239.255.255.250 port=5000