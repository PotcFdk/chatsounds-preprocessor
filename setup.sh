#!/bin/bash

# check for required commands

hash unzip 2>/dev/null || {
	echo >&2 "Missing 'unzip'.  Aborting."
	exit 1
}

hash wget 2>/dev/null || {
        echo >&2 "Missing 'wget'.  Aborting."
        exit 1
}

# detect architecture
if [ `getconf LONG_BIT` = "64" ]; then
    SYS_ARCH=x64
else
    SYS_ARCH=x86
fi

echo Architecture is $SYS_ARCH

wget --no-clobber http://uk.un4seen.com/files/bass24-linux.zip

mkdir -p lib

if [ $SYS_ARCH = "x64" ]; then
	unzip -o -j bass24-linux.zip "x64/libbass.so" -d lib
else
	unzip -o -j bass24-linux.zip "libbass.so" -d lib
fi

mkdir -p includes

unzip -o -j bass24-linux.zip "bass.h" -d includes
