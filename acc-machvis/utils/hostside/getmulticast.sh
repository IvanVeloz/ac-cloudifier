#!/bin/bash

# To be run on the development machine. Gets a multicast streaming video 
# from the camera.

gst-launch-1.0 -v \
    udpsrc \
        port=5000 \
        address=239.255.255.250 \
        caps="application/x-rtp, encoding-name=JPEG" \
    ! rtpjpegdepay \
    ! jpegparse \
    ! decodebin \
    ! videoconvert \
    ! autovideosink
