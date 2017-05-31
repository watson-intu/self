/**
* Copyright 2017 IBM Corp. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/


#include "utils/Log.h"
#include "utils/SelfException.h"
#include "utils/WatsonException.h"
#include "utils/ThreadPool.h"
#include "utils/JsonHelpers.h"
#include "SelfInstance.h"

#include <boost/filesystem.hpp>
#include <string.h>
#include <signal.h>
#include <fstream>

static void HandleIntSignal(int s)
{
	Log::Status( "Self", "Caught signal: %d, stopping self instance.", s );
	ThreadPool::Instance()->StopMainThread();
}

static void ValidateDirectory( const std::string & a_Dir )
{
	if (! boost::filesystem::is_directory( a_Dir.c_str() ) )
	{
		try {
			boost::filesystem::create_directories( a_Dir.c_str() );
		}
		catch( const std::exception & )
		{
			printf( "Error: Failed to validate directory (%s).\n", a_Dir.c_str() );
			exit(1);
		}
	}
}

int SELF_API SelfMain( int argc, char ** argv )
{
	std::string staticData = "./etc/";
	std::string instanceData = "./";

#if defined(_WIN32)
	std::string platform = "w32";
#elif defined(__APPLE__)
	std::string platform = "mac";
#else
	std::string platform = "linux";
#endif

	LogLevel eConsoleLevel = LL_STATUS;
	LogLevel eFileLevel = LL_STATUS;

	int newPort = 0;
	std::string newGateway, newGroupId, newName, newToken, newOrganizationId, newProfile;

	bool bSetDevVersion = false;
	bool bEnableDevVersion = false;
	for (int arg=1; arg < argc; ++arg)
	{
		if (argv[arg][0] == '-')
		{
			switch (argv[arg][1])
			{
			case 'P':		// set platform
			{
				if ((arg + 1) >= argc)
				{
					printf("Error: -P argument is missing.\n");
					return 2;
				}

				platform = argv[ arg + 1];
				arg += 1;
			}
			break;
			case 'u':		// set profile
			{
				if ((arg + 1) >= argc)
				{
					printf("Error: -u argument is missing.\n");
					return 2;
				}

				newProfile = argv[ arg + 1];
				arg += 1;
			}
			break;
			case 'c':		// set console level
			{
				if ((arg + 1) >= argc)
				{
					printf("Error: -c argument is missing.\n");
					return 2;
				}

				eConsoleLevel = (LogLevel)atoi(argv[arg + 1]);
				arg += 1;
			}
			break;
			case 'f':		// set file level
			{
				if ((arg + 1) >= argc)
				{
					printf("Error: -f argument is missing.\n");
					return 2;
				}

				eFileLevel = (LogLevel)atoi(argv[arg + 1]);
				arg += 1;
			}
			break;
			case 'p':
			{
				if ((arg + 1) >= argc )
				{
					printf( "Error: -p argument is missing.\n" );
					return 2;
				}

				newPort = atoi( argv[arg + 1] );
				arg += 1;
			}
			break;
			case 'g':
			{
				if ((arg + 1) >= argc)
				{
					printf("Error: -g argument is missing.\n");
					return 2;
				}

				newGateway = argv[arg + 1];
				arg += 1;
			}
			break;
			case 'k':
			{
				if ((arg + 1) >= argc)
				{
					printf("Error: -k argument is missing.\n");
					return 2;
				}

				newGroupId = argv[arg + 1];
				arg += 1;
			}
			break;
			case 'n':
			{
				if ((arg + 1) >= argc)
				{
					printf("Error: -n argument is missing.\n");
					return 2;
				}

				newName = argv[arg + 1];
				arg += 1;
			}
			break;
			case 't':
			{
				if ((arg + 1) >= argc)
				{
					printf("Error: -t argument is missing.\n");
					return 2;
				}
				newToken = argv[arg + 1];
				arg += 1;
			}
			break;
			case 'o':
			{
				if ((arg + 1) >= argc)
				{
					printf("Error: -o argument is missing.\n");
					return 2;
				}
				newOrganizationId = argv[arg + 1];
				arg += 1;
			}
			break;
			case 'd':
				bEnableDevVersion = true;
				bSetDevVersion = true;
				break;
			case 's':
			{
				if ( (arg + 1) >= argc )
				{
					printf( "Error: -s argument is missing.\n");
					return 2;
				}
				staticData = argv[arg + 1];
				arg += 1;
			}
			break;
			case 'i':
			{
				if ( (arg + 1) >= argc )
				{
					printf( "Error: -i argument is missing.\n");
					return 2;
				}
				instanceData = argv[arg + 1];
				arg += 1;
			}
			break;
			default:
				printf("Usage:\n"
					"self_instance [options]\n"
					"Options:\n"
					"-P <Platform> .. Specify Platform\n"
					"-u <profile> .. Specify Profile\n"
					"-s <Path> .. Static Data Path\n"
					"-i <Path> .. Instance Data Path\n"
					"-p <Port> .. Set Listen Port\n"
					"-c [0-6] .. Set Console Log Level\n"
					"-f [0-6] .. Set File Log Level\n"
					"-g <Gateway URL> .. Set the Gateway URL\n"
					"-k <Group Id> .. Set the Group Id\n"
					"-n <Name> .. Set the Embodiment Name\n"
					"-t <Bearer Token> .. Set the Bearer Token\n"
					"-o <Organization Id> .. Set the organization Id\n"
					"-d .. Enable Developer Version\n"
					);
				return 1;
			}
		}
		else
		{
			staticData = argv[arg];
			break;
		}
	}

	// ensure the paths are normalized and end with /
	StringUtil::Replace( staticData, "\\", "/" );
	if ( ! StringUtil::EndsWith( staticData, "/" ) )
		staticData += "/";
	StringUtil::Replace( instanceData, "\\", "/" );
	if ( ! StringUtil::EndsWith( instanceData, "/" ) )
		instanceData += "/";

	// ensure required directories exist
	ValidateDirectory( staticData );
	ValidateDirectory( instanceData );
	ValidateDirectory( instanceData + "cache" );
	ValidateDirectory( instanceData + "db" );

	Log::RegisterReactor(new ConsoleReactor(eConsoleLevel));
	Log::RegisterReactor(new FileReactor( (instanceData + "SelfInstance.log").c_str(), eFileLevel));

	// check for body.json, install into the instanceData directory if none is found or the static one is newer..
	std::string configFile( instanceData + "config.json" );

	Json::Value config( Json::objectValue );
	if ( boost::filesystem::exists( configFile ) && !JsonHelpers::Load( configFile, config ) )
		return 1;

	if (!newProfile.empty())
		config["m_Profile"] = newProfile;
	if (bSetDevVersion)
		config["m_bUseDevVersion"] = bEnableDevVersion;
	if (newName.size() > 0)
		config["m_Name"] = newName;
	if (newGroupId.size() > 0)
		config["m_GroupId"] = newGroupId;
	if (newToken.size() > 0)
		config["m_BearerToken"] = newToken;
	if (newOrganizationId.size() > 0)
		config["m_OrgId"] = newOrganizationId;
	if (newGateway.size() > 0 )
		config["m_GatewayURL"] = newGateway;
	if ( newPort != 0 )
		config["m_nPort"] = newPort;

	if (! JsonHelpers::Save( configFile, config ) )
		return 1;

	SelfInstance self( platform, staticData, instanceData);

	// handle control-C
	signal( SIGINT, HandleIntSignal );

	int rv = self.Run();

	Log::RemoveAllReactors();

	return rv;
}

