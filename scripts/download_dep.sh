#!/bin/bash
# Usage: install_tc.sh.sh <target>  
# Install toolchain for the given target

export TARGET=$1
STORE_URL="http://75.126.4.99/xray/?action="

# download target specific pagaes
if [ "$TARGET" == "mac" ]; then
		curl "${STORE_URL}/download?packageId=portaudio-osx64-0.1.zip" --output portaudio-osx64-0.1.zip
elif [ "$TARGET" == "nao" ]; then
		curl "${STORE_URL}/download?packageId=openssl-i686-aldebaran-linux-gnu-1.0.1s.zip" --output openssl-nao.zip
elif [ "$TARGET" == "raspi" ]; then
		curl "${STORE_URL}/download?packageId=boost-raspi-0.1.zip" --output boost-raspi-0.1.zip
elif [ "$TARGET" == "linux" ]; then
		echo There are no dependencies for Linux
fi

