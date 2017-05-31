# Intu
 Intu is an agent based embodiment middleware for devices. It integrates inputs such as microphones, cameras, and other sensors together with cognitive services and outputs such as joints and audio outputs. 

## Before you begin

  * Ensure your hardware meets the following requirements:
  
    * Windows
    
      * Intel® Core 2 or AMD Athlon® 64 processor; 2 GHz or faster processor
      * Microsoft Windows 7 with Service Pack 1, Windows 8.1, or Windows 10, Windows 2013
      * 2 GB of RAM (8 GB recommended)
      * 1 GB of available hard-disk space for 32-bit installation; 1 GB of available hard-disk space for 64-bit installation; additional free space required during installation (cannot install on a volume that uses a case-sensitive file system)
      * 1024 x 768 display (1280x800 recommended) with 16-bit color and 512 MB of dedicated VRAM; 2 GB is recommended
      * OpenGL 2.0–capable system
      * CPU: SSE2 instruction set support
      * Graphics card: DX9 (shader model 3.0) or DX11 with feature level 9.3 capabilities.
      * Internet connection and registration are necessary for required software activation, validation of subscriptions, and access to online services.

    * Mac OS
    
      * Multicore Intel processor with 64-bit support
      * Mac OS 10.12, 10.11, 10.10, 10.9
      * 2 GB of RAM (8 GB recommended)
      * 1GB of available hard-disk space for installation; additional free space required during installation (cannot install on a volume that uses a case-sensitive file system)
      * 1024 x 768 display (1280x800 recommended) with 16-bit color and 512 MB of dedicated VRAM; 2 GB is recommended
      * OpenGL 2.0–capable system (This is must have for Unity Application)
      * CPU: SSE2 instruction set support
      * Graphics card: DX9 (shader model 3.0) or DX11 with feature level 9.3 capabilities.
      * Internet connection and registration are necessary for required software activation, membership validation, and access to online services.

## Compiling Intu for various platforms

### Getting the files

Clone the Intu git repository onto your local machine by running the following two commands in your terminal:

  * `git clone --branch master --recursive git@github.com:watson-intu/self.git`
  * `git submodule update --init --recursive`

### Release Notes
* You may need to delete the "wdc" directory to get latest due to that directory being converted into a sub-module.
* If you use git bash to checkout, run the following command to checkout the submodule `git submodule foreach git pull origin develop`.

### Windows

1. Set up [Visual Studio 2015](https://www.visualstudio.com/downloads/).
2. Open `/vs2015/self.sln` in this project.
3. Select self_instance as your startup project, compile, and run.

### OS X

1. Set up [CMake](http://doc.aldebaran.com/2-1/dev/cpp/install_guide.html#required-buidsys). To install CMake by using Homebrew, run `brew install cmake`.
  * To install Homebrew, run the following command in your terminal: ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
2. Set up [qiBuild](http://doc.aldebaran.com/2-1/dev/cpp/install_guide.html#qibuild-install).
  * `pip install qibuild` (NOTE: it is highly recommended to download [anaconda python version 2.7](https://www.continuum.io/downloads) to have pip correctly configured)
  * `qibuild config --wizard` (use default setup for steps by pressing 1 twice)
3. Run the following commands:
  * `cd {self root directory}`
  * `./scripts/build_mac.sh`
  
This process stages the executables in the `bin/mac` directory on your local computer. You can change into that directory and run the unit_test and self_instance executables.

PS: If you run into issues with the build, you might have to change a couple of Boost header files, as described here: https://github.com/Homebrew/legacy-homebrew/issues/27396 (specifically, you might have to replace your copy of Boost's boost/atomic/detail/cas128strong.hpp and boost/atomic/detail/gcc-atomic.hpp with the latest available in the Boost directory)

### Linux

1. Set up qibuild and CMake. You can use your Linux package manager to install CMake, and any distribution of Python (2.7 recommended) to install qibuild through pip.
2. Download the [Linux Toolchain for Linux (C++ SDK 2.1.4 Linux 64)](https://community.aldebaran.com/en/resources/software) and unzip the package into `~/toolchains/naoqi-sdk-linux64/`.
3. Run the following commands:
  * `./scripts/build_linux.sh`

### Raspberry Pi

**Note:** If any step below fails or errors, run: `sudo apt-get update`, then repeat the step.

1.	Install Raspbian Operating System onto your Raspbery Pi

2.	Open up a new browser window on your laptop and download [**Anaconda 4.2.0 For Linux Python 2.7 version**](https://www.continuum.io/downloads).

	**Make sure you download the correct version.** You need the LINUX version no matter what operating system you have. Windows users may have to right click and select **Save as** to save the download locally.

3.	Copy Anaconda from your laptop over to the Pi. **Windows** users may need to [download](https://filezilla-project.org/) and use something like **Filezilla** to copy files over to the Pi.
	1. Navigate to the directory where you downloaded Anaconda locally.
	2. Copy Anaconda from your laptop over to the Pi using the following command: `scp Anaconda2-4.2.0-Linux-x86.sh pi@{ip}:/home/pi` 
		(e.g. `scp Anaconda2-4.2.0-Linux-x86.sh pi@10.0.1.2:/home/pi`)
		If prompted, the username is **pi** and password is **raspberry**

4.	Install Anaconda on your Pi and set up qiBuild.
	1. In a new Terminal/PuTTY window, ssh into your Pi: `ssh pi@{ip_address}`. When prompted, the username is **pi** and password is **raspberry**.
	2.	Run: `bash Anaconda2-4.2.0-Linux-x86.sh`.
	3. Follow the steps on the screen to install Anaconda. When you get to the license, keep hitting Enter to jump to the bottom. Type **yes** to approve the license.
	4.	Hit Enter to install Anaconda in the default location. **Note**: It may take a while for the progress to update, and if you get the following error, please ignore it.

		```
Anaconda2-4.2.0-Linux-x86.sh: line 484: /home/pi/anaconda2/pkgs/python-3.5.2-0/bin/python: cannot execute binary file: Exec format error
ERROR:
cannot execute native linux-32 binary, output from 'uname -a' is:
Linux raspberrypi 4.4.21-v7+ #911 SMP Thu Sep 15 14:22:38 BST 2016 armv7l GNU/Linux
		```

	5. Once Anaconda has successfully installed, run: `sudo apt-get install python-pip cmake`. 
	
		**Note:** If this fails, run `sudo apt-get update`, and then rerun: `sudo apt-get install python-pip cmake`.

	6.	Run: `sudo pip install qibuild`
 
5.	Install the wiringPi library on the Pi.
	1. In a new Terminal/PuTTY window, ssh into your Pi: `ssh pi@{ip_address}`. 
	2.	Navigate to your Pi's **home directory** by running: `cd /home/pi`. 
	3.	Run: `git clone git://git.drogon.net/wiringPi`
	4.	Now navigate into the wiringPi directory by running: `cd wiringPi/`
	5.	Run: `./build`

	You should see a list of classes compiled and “All Done” at the end.

6.	Build Intu on the Pi
	1. In your current (or a new) ssh session to the Raspberry Pi, navigate to the self directory: `cd {self root directory}`
	2. Mark the build script as executable by running: `chmod +x scripts/build_raspi.sh`
	3. Now build the Self SDK by running: `scripts/build_raspi.sh`

	**Note:** If you have any build errors, run: `scripts/clean.sh` and then rerun: `scripts/build_raspi.sh`

7. 	Run Intu on the Pi
	1. If you have a HDMI cable plugged into your Raspberry Pi, verify that the sound is set to analog. This can be done by right clicking the speaker icon at the top right hand corner of the Raspberry Pi's homescreen, and selecting analog.

	2. Verify that you have a microphone and speaker plugged into your Raspberry Pi. Note that your speaker may need to be charged before use. Make sure that it is turned on before proceeding with the next step.

	3. Navigate to the raspi directory using: `cd {self root directory}/bin/raspi`.
	
	4. Run: `./run_self.sh`
    
This process installs Intu on the remote device whose user name and IP address you provide. You can go to the `~/self/latest` directory on that device and run `run_self.sh`. This process was tested on Red Hat Enterprise 6.6 and 6.7.

### Aldebaran Nao and Pepper robots using OS X

1. Set up [CMake](http://doc.aldebaran.com/2-1/dev/cpp/install_guide.html#required-buidsys).
2. Set up [qiBuild](http://doc.aldebaran.com/2-1/dev/cpp/install_guide.html#qibuild-install).
3. Run the following commmands:
  * `./scripts/build_nao.sh`
4. Run the following command to install into on the given robot using scp:
  * `./scripts/install_nao.sh [user@host]`

  
## Getting Started
After downloading and building the code base, you can now run **Intu**. 

**Windows**: To run Intu on Windows, click the play button in Visual Studio. A web browser will open.

**Mac**: To run Intu on Mac, from your Intu directory, navigate to the `bin/mac` directory and execute the following command: `./run_self.sh`. A web browser will open.
 

## Configuring Intu
After initially running Intu, you will now need to configure Intu to use your own services in the web browser that opened.  While it is not required, we recommend using IBM Bluemix services. To register for a Bluemix account, go to `https://console.ng.bluemix.net/registration/`. 


At a minimum you will need a **Conversation** service, a **Speech to Text** service, and a **Text to Speech** service to use Intu.  If you are using IBM Bluemix's Conversation service, we have provided a sample workspace you can import and use.  For further instructions on how to use this workspace, jump to the [Getting Started with Conversation](#Conversation) section below.

To configure your services:

1. Click on the Menu button located on the left side of your browser.
2. Click on the Configure icon (the second icon from the top, above Logs).
3. Click on the `SERVICES CONFIG` button. You will be directed to a Services Config page.
4. Click on the **+** button next to the service you are configuring. An expanded view will appear.
5. Enter a username and password (if the service has one).  Click save.
6. **Reminder**: At a minimum you need to configure the following three services: **ConversationV1**, **SpeechToTextV1**, and **TextToSpeechV1**.  All other services are optional but can add extra functionality to Intu.
7. For your Conversation service, you will also need to add a workspace id.  Click on the Menu button -> Configure icon (second icon from the top) -> CLASSIFIERS button. You will be directed to the Classifiers configuration page.
8. Click on the **+** button for **TextClassifier**.  An expanded view will appear.
9. Search for the `self_dialog` workspace key.  Above it you will see a `m_WorkspaceId` field.  Input your own workspace id in this field.  Click save.
10. Your instance of Intu is now ready to use.



### <a name="Conversation">Getting started with Conversation</a>

Located in the `etc/shared` directory is a Conversation service workspace named **intu\_conversation\_v1.json**. This workspace helps you visualize how intents, entities, and dialog are developed. You can expand on this workspace or use it as a guide for developing your own later.  To import this workspace, follow these steps:

1. Navigate to your Conversation service.
2. Click **Launch tool**.
3. You will be directed to your workspace dashboard.
4. Click on the import icon (located next to the `Create` button).
5. Select the Conversation workspace json we have provided: `etc/shared/intu_conversation_v1.json`.
6. This Conversation workspace will automatically open once it is imported successfully.
7. You may now edit or view this Conversation workspace.

  
## Feedback

Post your comments and questions and include the `project-intu` and `intu` tags on 
[dW Answers](https://developer.ibm.com/answers/questions/ask/?topics=watson)
or [Stack Overflow](http://stackoverflow.com/questions/ask?tags=ibm-watson).
