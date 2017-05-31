#!/bin/bash

SELF_HOME="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SELF_PATH="$SELF_HOME/.."
SCRIPT="$SELF_HOME/run_self.py"
PLATFORM='raspi'
UPDATER_NAME='Self-${PLATFORM}.zip'
PREV_STATUS=0
AUTOSTART="/home/pi/.config/lxsession/LXDE-pi/autostart"
	
while true; do
	if [ "${PREV_STATUS}" -eq "2" ]; then
		echo Going to need to reinstall self!
		fileSize=$(stat -c%s $UPDATER_NAME)
		#Need to check for self zip file
		if [ ! -f $UPDATER_NAME] && [ ${fileSize} -gt "0" ]; then
			echo "Expected a new version of Self, but none could be found!"
			PREV_STATUS=0
			continue
		fi

		self_version="$(cat SelfVersion)"

		#Need to confirm that we were able to successfully unzip that file
		
		# remove any previous directory first
		if [ -d ${SELF_PATH}/${self_version} ]; then
			rm -rf ${SELF_PATH}/${self_version}
		fi
		# remake the directory and move the zip into the new directory
		mkdir ${SELF_PATH}/${self_version}
		mv $UPDATER_NAME ${SELF_PATH}/${self_version}
		
		# extract the files
		cd ${SELF_PATH}/${self_version}
		unzip $UPDATER_NAME
		extraction_successful="$(echo $?)"
		if [ ${extraction_successful} -ne "0" ]; then
			echo "Problem with extraction, probably a malformed download. Abort!"
			PREV_STATUS=0
			continue
		fi
		# copy over the previous config.json
		cp -rf ${SELF_PATH}/latest/config.json ${SELF_PATH}/${self_version}/
		
		#Need to recreate sym links
		echo "Recreating sym link"
		cd ${SELF_PATH}/
		rm ${SELF_PATH}/latest
		ln -s ${SELF_PATH}/${self_version}/ ${SELF_PATH}/latest
		SELF_HOME=${SELF_PATH}/${self_version}
	fi

	echo Configuring auto start of intu
	if grep -q "run_self.sh" ${AUTOSTART}
	then
  		echo "Auto start of intu is already configured!"
	else
		echo "/home/pi/self/latest/run_self.sh" >>  ${AUTOSTART}
	fi

	echo Running self..
	ulimit -c unlimited
	export LD_LIBRARY_PATH=${SELF_HOME}
	export WIRINGPI_GPIOMEM=1
	cd "${SELF_HOME}"
	./self_instance -P ${PLATFORM} "$@"
	PREV_STATUS="$(echo $?)"
	if [ -a core ]; then
		echo "self_instance crash, backtrace:" > backtrace.log
		gdb --batch --quiet -ex "thread apply all bt full" -ex "quit" ./self_instance core >> backtrace.log
		# TODO: send backtrace using curl through a webinterface
		rm core
		sleep 5
	else
		if [ "${PREV_STATUS}" -eq "0" ]; then
			exit
		fi
	fi
done
