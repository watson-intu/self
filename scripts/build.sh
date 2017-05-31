#!/bin/bash
# Usage: build.sh <platform>

export TARGET=$1
TC_NAME="${TARGET}"

if [ "$ACTION" == "" ]; then
	ACTION=make
fi

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR=$DIR/..
PACKAGES_DIR=$BUILD_DIR/packages

cd "$BUILD_DIR"

if [ ! -d "$BUILD_DIR/.qi" ]; then
	qibuild init
fi

qitoolchain info $TC_NAME
if [ $? != 0 ]; then 
	"$DIR"/tc_install.sh $TARGET
	if [ $? != 0 ]; then 
		echo "Failed to install toolchain"
		exit 1
	fi
fi

cd "$BUILD_DIR"
qibuild configure -c "$TC_NAME"
if [ $? -ne 0 ]; then exit 1; fi
qibuild $ACTION -c "$TC_NAME" -j 4
if [ $? -ne 0 ]; then exit 1; fi

"$DIR"/stage.sh $1 
