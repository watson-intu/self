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


#include "utils/ThreadPool.h"
#include "utils/GetMac.h"
#include "utils/StringUtil.h"
#include "topics/TopicManager.h"

#include <list>

#ifdef _WIN32
#include <conio.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>

#define getch getchar

void changemode(int dir)
{
  static struct termios oldt, newt;
 
  if ( dir == 1 )
  {
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
  }
  else
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}
 
int kbhit (void)
{
  struct timeval tv;
  fd_set rdfs;
 
  tv.tv_sec = 0;
  tv.tv_usec = 0;
 
  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);
 
  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &rdfs);
 
}

#endif

#pragma warning(disable:4996)

struct ClientConfig : public ISerializable
{
	RTTI_DECL();

	ClientConfig() : m_nPort( 9494 )
	{}

	std::string	m_Token;
	std::string m_EmbodimentId;
	int m_nPort;

	virtual void Deserialize(const Json::Value & json)
	{
		m_Token = json["m_EmbodimentCreds"]["m_BearerToken"].asString();
		m_EmbodimentId = json["m_EmbodimentId"].asString();
		m_nPort = json["m_pTopicManager"]["m_nPort"].asInt();
	}
	virtual void Serialize(Json::Value & json)
	{}
};

RTTI_IMPL( ClientConfig, ISerializable );

class TopicClient : public ConsoleReactor
{
public:
	TopicClient() : m_sTarget( "." ), m_bQuit( false )
	{
		Log::RegisterReactor( this );
	}
	~TopicClient()
	{
		Log::RemoveReactor( this, false );
	}

	//! ILogReactor 
	virtual void Process(const LogRecord & a_Record)
	{
		DeleteCommandLine();
		ConsoleReactor::Process( a_Record );
		PrintCommandLine();
	}
	
	int Run( int argc,char ** argv )
	{
		std::string parentHost, token, selfId;

		if ( argc > 1 )
		{
			for(int i=1;i<argc;++i)
			{
				if ( argv[i][0] == '-' )
				{
					switch( argv[i][1] )
					{
					case 't':
						if ( (i+1) >= argc )
						{
							printf( "Error: Missing argument for -t\r\n" );
							return 1;
						}
						token = argv[++i];
						break;
					case 's':
						if ( (i+1) >= argc )
						{
							printf( "Error: Missing argument for -s\r\n" );
							return 1;
						}
						selfId = argv[++i];
						break;
					case 'h':
						if ( (i+1) >= argc )
						{
							printf( "Error: Missing argument for -h\r\n" );
							return 1;
						}
						parentHost = argv[++i];
						break;
					case '?':
						printf( "Usage: topic_clinet [-t <token>] [-s <selfId>] [-h <host>]\r\n" );
						return 1;
					}
				}
			}
		}

		if ( parentHost.size() == 0 || token.size() == 0 || selfId.size() == 0 )
		{
			// try to load settings from the body.. 
			ClientConfig config;
			if ( ISerializable::DeserializeFromFile( "body.json", &config ) == NULL )
			{
				printf( "Error: Failed to load body.json.\r\n" );
				return 1;
			}

			if ( token.size() == 0 )
				token = config.m_Token;
			if ( selfId.size() == 0 )
				selfId = config.m_EmbodimentId;
			if ( parentHost.size() == 0 )
				parentHost = StringUtil::Format( "ws://127.0.0.1:%d", config.m_nPort );
		}

		m_TopicMgr.SetBearerToken( token );
		m_TopicMgr.SetSelfId( selfId );
		m_TopicMgr.SetParentHost( parentHost );

		if (! m_TopicMgr.Start() )
		{
			printf( "Error: Failed to start topic agent.\r\n" );
			return 1;
		}

		printf( "Connected to %s as %s\r\n", 
			parentHost.c_str(), selfId.c_str() );
		printf( "Enter 'help' for a list of commands.\r\n\r\n" );

		PrintCommandLine();

		while(! m_bQuit )
		{
			ThreadPool::Instance()->ProcessMainThread();
			UpdateCommandLine();

			tthread::this_thread::yield();
		}

		return 0;
	}

	void PrintCommandLine()
	{
		// output new line...
		std::cout << "[" + m_sTarget + "]>" + m_sInput;
	}
	void DeleteCommandLine()
	{
		// remove previous input..
		for(size_t i=0;i<(m_sPrevInput.size() + m_sTarget.size() + 3);++i)
			std::cout << "\b \b";
	}

	std::string GetPath(const std::string & a_Target, const std::string & a_Topic)
	{
		std::string sPath;
		size_t nLastDot = a_Target.find_last_of("/.");
		if (nLastDot != std::string::npos)
			sPath = a_Target.substr(0, nLastDot) + a_Topic;
		else
			sPath = a_Topic;

		return sPath;
	}

	void UpdateCommandLine( bool a_bForceUpdate = false )
	{
		bool bExecuteCommand = false;
		if ( kbhit() )
		{
			unsigned char key = getch();
			if ( key == '\r' || key == '\n' )
				bExecuteCommand = true;
			else if ( key == '\b' && m_sInput.size() > 0 ) // backspace
			{
				DeleteCommandLine();
				m_sInput = m_sInput.substr( 0, m_sInput.size() - 1 );
				PrintCommandLine();
			}
			else if ( isprint( key ) )
				m_sInput += key;
		}

		if ( bExecuteCommand )
		{
			std::cout << "\r\n";

			std::vector<std::string> parts;
			StringUtil::Split( m_sInput, " ", parts );
			m_sInput.clear();

			if ( parts.size() > 0 )
			{
				std::string command( parts[0] );
				StringUtil::ToLower( command );

				if ( command == "quit" )
					m_bQuit = true;
				else if ( command == "?" || command == "help" )
				{
					std::cout << "Commands:\r\n";
					std::cout << "quit ... Quit this client.\r\n";
					std::cout << "sub <topic> ... Subscribe to the given topic on the current target.\r\n";
					std::cout << "unsub <topic> ... Unsubscribe from the given topic.\r\n";
					std::cout << "query ... Query the current target.\r\n";
					std::cout << "target <target> ... Change the current target.\r\n";
					std::cout << "say <message> ... Post text on the blackboard of target.\r\n";
					std::cout << "emote <emotion> ... Post emote on the blackboard of target.\r\n";
				}
				else if ( command == "query" )
				{
					printf( "Sending query to %s...\r\n", m_sTarget.c_str() );
					m_TopicMgr.Query( m_sTarget, DELEGATE( TopicClient, OnQueryInfo, const ITopics::QueryInfo &, this ) );
				}
				else if ( command == "target" )
				{
					if ( parts.size() > 1 )
						m_sTarget = parts[1];
					else
						m_sTarget = ".";

					printf( "Target set to '%s'...\r\n", m_sTarget.c_str() );
				}
				else if ( command == "sub" )
				{
					if ( parts.size() > 1 )
					{
						std::string sPath(GetPath(m_sTarget, parts[1] ));
						m_TopicMgr.Subscribe( sPath, DELEGATE( TopicClient, OnPayload, const ITopics::Payload &, this ) );
					}
					else
						printf( "Usage: sub <topic>\r\n" );
				}
				else if (command == "unsub")
				{
					if (parts.size() > 1)
					{
						std::string sPath(GetPath(m_sTarget, parts[1]));
						m_TopicMgr.Unsubscribe(sPath);
					}
					else
						printf("Usage: unsub <topic>\r\n");
				}
				else if (command == "say")
				{
					std::string message;
					for (size_t i = 1; i < parts.size(); ++i)
					{
						if (message.size() > 0)
							message += " ";
						message += parts[i];
					}

					if (message.size() > 0)
					{
						std::string sPath(GetPath(m_sTarget, "conversation"));
						m_TopicMgr.PublishAt(sPath, message);
					}
					else
						printf("Usage: say <message>\r\n");
				}
				else if (command == "emote")
				{
					if (parts.size() > 1)
					{
						Json::Value text;
						text["Type_"] = "Emotion";
						text["m_Type"] = parts[1];
						Json::Value json;
						json["thing"] = text;

						std::string sPath(GetPath(m_sTarget, "blackboard"));
						m_TopicMgr.PublishAt(sPath, json.toStyledString());
					}
					else
						printf("Usage: emote <emotion>\r\n");
				}
			}

			a_bForceUpdate = true;
		}

		if ( m_sPrevInput != m_sInput || a_bForceUpdate )
		{
			DeleteCommandLine();
			PrintCommandLine();
			m_sPrevInput = m_sInput;
		}
	}

	void OnQueryInfo( const ITopics::QueryInfo & a_Info )
	{
		DeleteCommandLine();

		if ( a_Info.m_bSuccess )
		{
			printf( "Received query response:\r\n" );
			printf( "Path: %s\r\n", a_Info.m_Path.c_str() );
			printf( "SelfId: %s\r\n", a_Info.m_SelfId.c_str() );
			printf( "ParentId: %s\r\n", a_Info.m_ParentId.c_str() );
			printf( "Name: %s\r\n", a_Info.m_Name.c_str() );
			printf( "Type: %s\r\n", a_Info.m_Type.c_str() );

			printf( "%lu Children:\r\n", a_Info.m_Children.size() );
			for(size_t i=0;i<a_Info.m_Children.size();++i)
				printf( "\t%s\r\n", a_Info.m_Children[i].c_str() );

			printf( "%lu Topics:\r\n", a_Info.m_Topics.size() );
			for(size_t i=0;i<a_Info.m_Topics.size();++i)
				printf( "\t%s (%s)\r\n", a_Info.m_Topics[i].m_TopicId.c_str(), a_Info.m_Topics[i].m_Type.c_str() );
		}
		else
			std::cout << "Query failed...!\r\n";

		PrintCommandLine();
	}

	void OnPayload( const ITopics::Payload & a_Payload )
	{
		Log::Status( "TopicClient", "\r\nReceive Payload for topic: %s, origin: %s:\r\n%s\r\n----------------------------", 
			a_Payload.m_Topic.c_str(), a_Payload.m_Origin.c_str(), a_Payload.m_Data.c_str() );
	}

	//! Data
	bool			m_bQuit;
	std::string		m_sTarget;
	std::string		m_sInput;
	std::string		m_sPrevInput;
	TopicManager	m_TopicMgr;
};

int main( int argc, char ** argv )
{
	ThreadPool pool;
	TimerPool timers;

	Log::RegisterReactor(new FileReactor("TopicClient.log"));

#ifndef _WIN32
	changemode(1);
#endif
	TopicClient client;
	int rv = client.Run( argc, argv );
#ifndef _WIN32
	changemode(0);
#endif
	return rv;
}
