#!/bin/bash

P=$(dirname $0)
${P}/makelinux.sh
valgrind --tool=memcheck --leak-check=full ${P}/bin/acc-control
