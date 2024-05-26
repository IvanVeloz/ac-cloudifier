#!/bin/sh

# To be run on the gadget. Configures a handful of critical camera parameters for a proper picture.

v4l2-ctl --set-ctrl power_line_frequency=2,auto_exposure=1,exposure_time_absolute=400