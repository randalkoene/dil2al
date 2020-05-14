#! /bin/sh
#
# gcc version specific environment variable settings
#
# Randal A. Koene, 20041218

echo "Setting up gcc version specific environment variables"

GCCVERSION=`gcc -v 2>&1 | grep "^gcc version[^0-9]*[3-9][.]"`
if [ "$GCCVERSION" = "" ]; then
    GCCVERSION="2.x_orless"
else
    GCCVERSION="3.x_ormore"
fi

export GCCVERSION

if [ "$GCCVERSION" = "3.x_ormore" ]; then
    GCC3FXTRA="-Wno-unused-function"
    export GCC3FXTRA
fi
