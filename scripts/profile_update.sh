#!/bin/bash
# Usage: profile_update_.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR=$DIR/..
PROFILES_DIR=$BUILD_DIR/profiles
VERSION="$(cat $BUILD_DIR/version.txt)"
STORE_URL="http://75.126.4.99/xray/?action="

cd $PROFILES_DIR/
curl -u labDemo:demo2000 -i "${STORE_URL}/setVersions&packageId=Self-Profile.zip.json&alphaVersion=$VERSION&devVersion=$VERSION&recVersion=$VERSION"


