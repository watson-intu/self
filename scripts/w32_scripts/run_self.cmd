REM run_self.cmd

SET SELF_HOME=%~dp0
SET SELF_PATH=%SELF_HOME%../
SET PLATFORM=w32
SET UPDATER_NAME=Self-%PLATFORM%.zip
SET PREV_STATUS=0

:loop
	if "%PREV_STATUS%" == "2" (
		SET /p NEW_VERSION=<SelfVersion
		
		echo Installing version %NEW_VERSION%...
		if NOT EXIST "%UPDATER_NAME%" (
			echo "Expected a new version of Self, but none could be found!"
			SET PREV_STATUS=0
			goto run_self
		)


		REM Need to confirm that we were able to successfully unzip that file
		SET NEW_HOME=%SELF_PATH%%NEW_VERSION%
		rmdir /Q /S "%NEW_HOME%"
		mkdir "%NEW_HOME%"
		
		move /Y "%UPDATER_NAME%" "%NEW_HOME%"

		cd "%NEW_HOME%"
		unzip "%UPDATER_NAME%" 
		if "%ERRORLEVEL%" NEQ "0" (
			echo "Problem with extraction, probably a malformed download. Abort!"
			SET PREV_STATUS=0
			goto run_self
		)

		REM  copy over the previous config.json
		copy  "%SELF_HOME%config.json" "%NEW_HOME%"
		REM  set the new home
		SET SELF_HOME=%NEW_HOME%
	)

:run_self
	REM create the desktop link to run this instance..
	SET DESKTOP_LINK="%USERPROFILE%\Desktop\Intu.url"
	echo [InternetShortcut] > "%DESKTOP_LINK%"
	echo URL=file:///%SELF_HOME%run_self.cmd >> "%DESKTOP_LINK%"
    echo IconIndex=0 >> "%DESKTOP_LINK%"
	echo IconFile=%SELF_HOME%run_self.cmd >> "%DESKTOP_LINK%"
	
	echo Running self..
	cd %SELF_HOME%
	self_instance -P %PLATFORM% %
	SET PREV_STATUS=%ERRORLEVEL%
	echo Self exited with status %PREV_STATUS%
	
	if "%PREV_STATUS%" == "0" exit /b 0
goto loop
