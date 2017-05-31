#!/bin/bash
which -s brew
if [[ $? != 0 ]] ; then
	if [ -d /usr/local/opt/portaudio ] ; then
		echo "Homebrew has not been installed, but portaudio folder is there, so we are updating files."
		cp libportaudio* /usr/local/Cellar/portaudio/19.20140130/lib/
	else
	    # There is no Homebrew - then we are moving files locally
	    echo "Homebrew has not been installed. We are placing audio libraries locally."
	    mkdir -p /usr/local/Cellar/portaudio/19.20140130/lib
		mkdir -p /usr/local/opt/
		ln -sf /usr/local/Cellar/portaudio/19.20140130 /usr/local/opt/portaudio
		cp libportaudio* /usr/local/Cellar/portaudio/19.20140130/lib/
	fi
else
    if brew list -1| grep -q portaudio; then 
    	# There is Homebrew - and portaudio
		echo "No need to install portaudio. It is already installed." 
	else 
		echo "portaudio has not been installed. Installing portaudio." 
		brew install portaudio
	fi
fi
