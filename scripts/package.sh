#!/bin/bash
# Usage: package.sh <target>  

export TARGET=$1

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR=$DIR/..
VERSION="$(cat $BUILD_DIR/version.txt)"
STORE_URL="http://75.126.4.99/xray/?action="
PACKAGES_DIR=$BUILD_DIR/packages

PACKAGE_ID=Self-$TARGET
PACKAGE_ZIP=$PACKAGE_ID.zip
PACKAGE_DIR=$PACKAGES_DIR/$PACKAGE_ID/

SDK_ID=Self-SDK-$TARGET
SDK_ZIP=$SDK_ID.zip
SDK_DIR=$PACKAGES_DIR/$SDK_ID/

cd $BUILD_DIR/
export ACTION=package
$DIR/build.sh $TARGET 
if [ $? -ne 0 ]; then
        exit 1
fi

# Stage the built binaries into the bin/$TARGET directory
# Install the public profile at the same time
$DIR/stage.sh $TARGET public
if [ $? -ne 0 ]; then
        exit 1
fi

rm -rf $PACKAGE_DIR
rm -rf $SDK_DIR

# Package the binaries
mkdir -p $PACKAGE_DIR
cp $BUILD_DIR/license.zip $PACKAGE_DIR
cp -R $BUILD_DIR/bin/$TARGET/* $PACKAGE_DIR

# Package the SDK
mkdir -p $SDK_DIR
cp $BUILD_DIR/license.zip $SDK_DIR

# Add any platform specific toolchain packages
mkdir -p $SDK_DIR/toolchain
cd $SDK_DIR/toolchain/
$DIR/download_dep.sh $TARGET
if [ $? -ne 0 ]; then exit 1; fi

# Move built package into place
mv $BUILD_DIR/package/*.zip .
rm -rf $BUILD_DIR/package

# copy binaries into our SDK package
mkdir -p $SDK_DIR/bin
cp -R $BUILD_DIR/bin/$TARGET $SDK_DIR/bin/

# make our package for uploading to the package store
cd $PACKAGES_DIR

rm $SDK_ZIP
rm $PACKAGE_ZIP

# Zip the binaries
cd $PACKAGE_ID
zip -r ../$PACKAGE_ZIP .
if [ $? != 0 ]; then exit 1; fi

# Zip the SDK
cd $PACKAGES_DIR
zip -r $SDK_ZIP $SDK_ID/ 
if [ $? != 0 ]; then exit 1; fi

# upload to package store
curl --progress-bar -u labDemo:demo2000 -i -F name=file -F filedata=@$PACKAGE_ZIP "${STORE_URL}/upload&packageId=$PACKAGE_ZIP&version=$VERSION"
curl -u labDemo:demo2000 -i "${STORE_URL}/setVersions&packageId=$PACKAGE_ZIP&alphaVersion=$VERSION&devVersion=$VERSION"
curl --progress-bar -u labDemo:demo2000 -i -F name=file -F filedata=@$SDK_ZIP "${STORE_URL}/upload&packageId=$SDK_ZIP&version=$VERSION"
curl -u labDemo:demo2000 -i "${STORE_URL}/setVersions&packageId=$SDK_ZIP&alphaVersion=$VERSION"

