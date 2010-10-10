#!/bin/sh
# Simple script to help format code to TMW-eA standards.
# You should use this on any file you edit before
# committing to git.
indent -nbad -bap -sc -bl -blf -bli0 -cli4 -cbi0 -di5 \
-nbc -bls -ip2 -nut -ts4 -bap -i4 -sob -npsl gui $*


