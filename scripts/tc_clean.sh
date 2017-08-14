#!/bin/bash
# Usage: clean.sh 

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR=$DIR/..

rm -rf $BUILD_DIR/packages
rm -rf $BUILD_DIR/.qi
rm -rf $BUILD_DIR/build*

qitoolchain remove nao --force
qitoolchain remove mac --force
qitoolchain remove raspi --force
qitoolchain remove linux --force

