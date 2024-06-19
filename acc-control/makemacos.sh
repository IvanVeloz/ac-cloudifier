#/bin/bash

make --debug \
CFLAGS+=-D_DESKTOP_BUILD_=1 \
CFLAGS+="-I$(brew --prefix mosquitto)/include"
