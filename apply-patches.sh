#!/bin/bash

env QUILT_PATCHES=debian/patches quilt push -a

RESULT=$?

if [ $RESULT -eq 0 -o $RESULT -eq 2 ]; then
    exit 0
fi

exit 1

