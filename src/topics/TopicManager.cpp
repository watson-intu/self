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


#include "TopicManager.h"
#include "services/IAuthenticate.h"
#include "SelfInstance.h"

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include <set>
#include <fstream>

const double PING_INTERVAL = 15.0;

REG_SERIALIZABLE(TopicManager);
RTTI_IMPL(TopicManager, IAgent);

TopicManager::TopicManager() : 
	m_InstanceId( UniqueID().Get() ),
	m_bActive(false),
	m_nPort( 9000 ), 
	m_nThreads( 5 ), 
	m_fRequestTimeout( 30.0f ),
	m_fReconnectInterval( 5.0f ),
	m_NextConnectionId( 1 ),
	m_NextSubscriberId( 1 ),
	m_NextRequestId( 1 ),
	m_MinLogLevel(LL_DEBUG_LOW),
	m_bProcessingLog(false)
{
	m_MessageHandlerMap["subscribe"] = DELEGATE(TopicManager, HandleSubscribe, const Json::Value &, this);
	m_MessageHandlerMap["subscribe_failed"] = DELEGATE(TopicManager, HandleSubscribeFailed, const Json::Value &, this);
	m_MessageHandlerMap["publish"] = DELEGATE(TopicManager, HandlePublish, const Json::Value &, this);
	m_MessageHandlerMap["publish_at"] = DELEGATE(TopicManager, HandlePublishAt, const Json::Value &, this);
	m_MessageHandlerMap["unsubscribe"] = DELEGATE(TopicManager, HandleUnsubscribe, const Json::Value &, this);
	m_MessageHandlerMap["no_route"] = DELEGATE(TopicManager, HandleNoRoute, const Json::Value &, this);
	m_MessageHandlerMap["query"] = DELEGATE(TopicManager, HandleQuery, const Json::Value &, this);
	m_MessageHandlerMap["query_response"] = DELEGATE(TopicManager, HandleQueryResponse, const Json::Value &, this);
	Log::RegisterReactor(this);
}

TopicManager::~TopicManager()
{
	Stop();
	Log::RemoveReactor(this, false);
}

void TopicManager::Serialize(Json::Value & json)
{
	json["m_CertFile"] = m_CertFile;
	json["m_KeyFile"] = m_KeyFile;
	json["m_VerifyFile"] = m_VerifyFile;
	json["m_ParentHost"] = m_ParentHost;
	json["m_Interface"] = m_Interface;
	json["m_nPort"] = m_nPort;
	json["m_nThreads"] = m_nThreads;
	json["m_fRequestTimeout"] = m_fRequestTimeout;
	json["m_fReconnectInterval"] = m_fReconnectInterval;
	json["m_RestUser"] = m_RestUser;
	json["m_RestPassword"] = Crypt::Encode(m_RestPassword);

	//SerializeMap( "m_TopicDataMap", m_TopicDataMap, json );
}

void TopicManager::Deserialize(const Json::Value & json)
{
	if ( json.isMember( "m_CertFile" ) )
		m_CertFile = json["m_CertFile"].asString();
	if ( json.isMember( "m_KeyFile" ) )
		m_KeyFile = json["m_KeyFile"].asString();
	if (json.isMember("m_VerifyFile"))
		m_VerifyFile = json["m_VerifyFile"].asString();
	if (json.isMember("m_ParentHost"))
		m_ParentHost = json["m_ParentHost"].asString();
	if ( json.isMember( "m_Interface" ) )
		m_Interface = json["m_Interface"].asString();
	if ( json.isMember( "m_nPort" ) )
		m_nPort = json["m_nPort"].asInt();
	if ( json.isMember( "m_nThreads" ) )
		m_nThreads = json["m_nThreads"].asInt();
	if (json.isMember("m_fRequestTimeout"))
		m_fRequestTimeout = json["m_fRequestTimeout"].asFloat();
	if (json.isMember("m_fReconnectInterval"))
		m_fReconnectInterval = json["m_fReconnectInterval"].asFloat();
	if ( json["m_RestUser"].isString() )
		m_RestUser = json["m_RestUser"].asString();
	if ( json["m_RestPassword"].isString() )
		m_RestPassword = Crypt::Decode(json["m_RestPassword"].asString());

	//DeserializeMap( "m_TopicDataMap", json, m_TopicDataMap );
}

void TopicManager::Process(const LogRecord & a_Record)
{
	if ( !m_bProcessingLog && a_Record.m_Level >= m_MinLogLevel && IsSubscribed("log") )
	{
		m_bProcessingLog = true;		// prevent a stack-overflow..

		Json::Value json;
		json["time"] = a_Record.m_Time;
		json["level"] = Log::LevelText( a_Record.m_Level );
		json["subsystem"] = a_Record.m_SubSystem;
		json["message"] = a_Record.m_Message;

		Publish( "log", json.toStyledString(), false );
		m_bProcessingLog = false;
	}
}

void TopicManager::SetLogLevel( LogLevel a_Level )
{
	m_MinLogLevel = a_Level;
}

bool TopicManager::Start()
{
	if ( ThreadPool::Instance() == NULL )
	{
		Log::Error( "TopicManager", "ThreadPool instance is NULL." );
		return false;
	}

	if (m_bActive)
		return false;		// we are already started
	m_bActive = true;

	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance != NULL )
	{
		m_SelfId = pInstance->GetLocalConfig().m_EmbodimentId;
		m_BearerToken = pInstance->GetLocalConfig().m_BearerToken;
	}

	Log::Status( "TopicManager", "Starting with selfId %s, bearer token %u bytes", 
		m_SelfId.c_str(), m_BearerToken.size() );

	if ( m_CertFile.size() > 0 && m_KeyFile.size() > 0 )
		m_spWebServer = IWebServer::SP( IWebServer::Create( m_CertFile, m_KeyFile, m_VerifyFile, m_Interface, m_nPort, m_nThreads, m_fRequestTimeout ) );
	else
		m_spWebServer = IWebServer::SP( IWebServer::Create( m_Interface, m_nPort, m_nThreads, m_fRequestTimeout ) );

	// add the end-points, order is important as they are evaluated in the order provided..
	m_spWebServer->AddEndpoint("/stream", DELEGATE(TopicManager, OnStreamRequest, IWebServer::RequestSP, this));
	m_spWebServer->AddEndpoint("/info", DELEGATE(TopicManager, OnInfoRequest, IWebServer::RequestSP, this));
	m_spWebServer->AddEndpoint("/topics/*", DELEGATE(TopicManager, OnTopicRequest, IWebServer::RequestSP, this));
	m_spWebServer->AddEndpoint("/www/*", DELEGATE(TopicManager, OnWWWRequest, IWebServer::RequestSP, this));

	if (!m_spWebServer->Start())
		Log::Error("TopicManager", "Failed to start web server.");

	if ( m_ParentHost.size() > 0 )
		ConnectToParent();

	RegisterTopic( "log", "application/json" );
	RegisterTopic( "topic-manager", "application/json" );
	Subscribe( "topic-manager", DELEGATE( TopicManager, OnTopicManagerEvent, const ITopics::Payload &, this ) );

	return true;
}

bool TopicManager::Stop()
{
	if (!m_bActive)
		return false;
	m_bActive = false;

	if (!m_spWebServer->Stop())
		Log::Error("TopicManager", "Error stopping web server.");

	if (m_spWebClient)
		m_spWebClient->Shutdown();

	// notify anyone who called RegisterTopic() that they no longer
	// have those subscribers.
	for( TopicMap::iterator iTopic = m_TopicMap.begin(); 
		iTopic != m_TopicMap.end(); ++iTopic )
	{
		StringList & subs = iTopic->second.m_Subscribers;
		for( StringList::const_iterator iSub = subs.begin(); iSub != subs.end(); ++iSub )
		{
			SubInfo info;
			info.m_Origin = *iSub;
			info.m_Subscribed = false;
			info.m_Topic = iTopic->second.m_Topic;

			iTopic->second.m_SubscriberCallback(info);
		}
		subs.clear();
	}

	m_spParentConnection.reset();
	m_spReconnectTimer.reset();
	m_SubscriptionMap.clear();
	m_SubscriberMap.clear();
	m_TopicMap.clear();
	m_ConnectionMap.clear();
	m_ConnectionLookup.clear();
	m_spWebClient.reset();
	m_spWebServer.reset();

	return true;
}

void TopicManager::GetTopics( TopicVector & a_Topics )
{
	for( TopicMap::const_iterator iTopic = m_TopicMap.begin(); iTopic != m_TopicMap.end(); ++iTopic )
		a_Topics.push_back( TopicInfo( iTopic->second.m_Topic, iTopic->second.m_Type ) );
}

void TopicManager::RegisterTopic( 
	const std::string & a_TopicId,
	const std::string & a_Type,
	SubCallback a_SubscriberCallback /*= SubCallback()*/ )
{
	m_TopicMap.insert( TopicPair( a_TopicId, TopicData( a_TopicId, a_Type, a_SubscriberCallback ) ) );
}

void TopicManager::UnregisterTopic( const std::string & a_TopicId)
{
	m_TopicMap.erase( a_TopicId );
}

bool TopicManager::IsSubscribed(const std::string & a_TopicId)
{
	TopicMap::iterator iTopic = m_TopicMap.find(a_TopicId);
	if (iTopic == m_TopicMap.end())
		return false;

	return iTopic->second.m_Subscribers.begin() != iTopic->second.m_Subscribers.end();
}

size_t TopicManager::GetSubscriberCount(const std::string & a_TopicId)
{
	TopicMap::iterator iTopic = m_TopicMap.find(a_TopicId);
	if (iTopic == m_TopicMap.end())
		return 0;

	return iTopic->second.m_Subscribers.size();
}

bool TopicManager::Publish( 
	const std::string & a_TopicId, 
	const std::string & a_Data, 
	bool a_bPersisted/* = false*/,
	bool a_bBinary /*= false*/ )
{
	TopicMap::iterator iTopic = m_TopicMap.find( a_TopicId );
	if ( iTopic == m_TopicMap.end() )
	{
		Log::Error( "TopicManager", "RegisterTopic() should be invoked before Publish() is invoked." );
		return false;
	}

	if ( a_bPersisted )
		m_TopicDataMap[ a_TopicId ] = a_Data;

	Json::Value json;
	json["targets"] = GetTargets(a_TopicId);
	json["origin"] = ".";
	json["msg"] = "publish";
	json["data"] = a_Data;
	json["persisted"] = a_bPersisted;
	json["binary"] = a_bBinary;
	json["topic"] = iTopic->second.m_Topic;
	json["type"] = iTopic->second.m_Type;
	RouteMessage(json);

	return true;
}

bool TopicManager::Send(
	const std::string & a_Target,
	const std::string & a_TopicId,
	const std::string & a_Data,
	bool a_bBinary /*= false*/ )
{
	TopicMap::iterator iTopic = m_TopicMap.find( a_TopicId );
	if ( iTopic == m_TopicMap.end() )
	{
		Log::Error( "TopicManager", "RegisterTopic() should be invoked before Send() is invoked." );
		return false;
	}

	Json::Value json;
	json["targets"][0] = a_Target;
	json["origin"] = ".";
	json["msg"] = "publish";
	json["data"] = a_Data;
	json["persisted"] = false;
	json["binary"] = a_bBinary;
	json["topic"] = iTopic->second.m_Topic;
	json["type"] = iTopic->second.m_Type;
	RouteMessage(json);

	return true;
}

bool TopicManager::PublishAt(
	const std::string & a_Path,
	const std::string & a_Data,
	bool a_bPersisted /*= false*/,
	bool a_bBinary /*= false*/ ) 
{
	Json::Value json;
	json["targets"][0] = a_Path;
	json["origin"] = ".";
	json["msg"] = "publish_at";
	json["data"] = a_Data;
	json["persisted"] = a_bPersisted;
	json["binary"] = a_bBinary;
	RouteMessage(json);

	return true;
}

void TopicManager::Query(
	const std::string & a_Path,				//! the path to the parent topic, we will return all topics underneath the provided topic.
	QueryCallback a_Callback )
{
	unsigned int reqId = m_NextRequestId++;
	m_QueryRequestMap[reqId] = QueryRequest(a_Path, a_Callback);

	Json::Value json;
	json["targets"][0] = a_Path;
	json["origin"] = ".";
	json["msg"] = "query";
	json["request"] = reqId;
	RouteMessage(json);
}

bool TopicManager::Subscribe(
	const std::string & a_Path,	
	PayloadCallback a_Callback)
{
	m_SubscriptionMap[a_Path].push_back( Subscription( a_Path, a_Callback ) );

	Json::Value json;
	json["targets"][0] = a_Path;
	json["origin"] = ".";
	json["msg"] = "subscribe";

	RouteMessage(json);
	return true;
}

bool TopicManager::Unsubscribe( const std::string & a_Path, void * a_pObject /*= NULL*/ )
{
	SubscriptionMap::iterator iSubscription = m_SubscriptionMap.find(a_Path);
	if (iSubscription == m_SubscriptionMap.end())
		return false;

	bool bSuccess = false;

	SubscriptionList & subs = iSubscription->second;
	for( SubscriptionList::iterator iSub = subs.begin(); iSub != subs.end(); )
	{
		if ( a_pObject == NULL || iSub->m_Callback.IsObject( a_pObject ) )
		{
			subs.erase( iSub++ );
			bSuccess = true;
		}
		else
			++iSub;
	}

	// if that was the last subscriber, then send a message so the publisher will stop sending messages.
	if ( subs.begin() == subs.end() )
	{
		m_SubscriptionMap.erase( iSubscription );

		Json::Value json;
		json["targets"][0] = a_Path;
		json["origin"] = ".";
		json["msg"] = "unsubscribe";

		RouteMessage(json);
	}

	return bSuccess;
}

//--------------------------------

bool TopicManager::Authenticate(IWebServer::RequestSP a_spRequest)
{
	if ( m_RestUser.size() > 0 )
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();

		IWebServer::Headers::iterator iAuth = a_spRequest->m_Headers.find( "Authorization" );
		if ( iAuth != a_spRequest->m_Headers.end() )
		{
			// Basic <base64 encoded data>
			std::vector<std::string> parts;
			StringUtil::Split( iAuth->second, " ", parts );

			if ( parts.size() == 2 && StringUtil::Compare( parts[0], "basic", true ) == 0 )
			{
				std::string sAuth( StringUtil::DecodeBase64( parts[1] ) );

				size_t nSeperator = sAuth.find_first_of( ':' );
				if ( nSeperator != std::string::npos )
				{
					std::string sUser = sAuth.substr( 0, nSeperator );
					std::string sPassword = sAuth.substr( nSeperator + 1 );

					if ( pInstance != NULL
						&& parts.size() == 2 
						&& sUser == m_RestUser
						&& sPassword == m_RestPassword )
					{
						// authenticated!
						return true;
					}
				}
				else
					Log::Warning( "TopicManager", "Malformed basic authentication: %s", iAuth->second.c_str() );
			}
			else
				Log::Warning( "TopicManager", "Unsupported authorization method: %s", iAuth->second.c_str() );
		}

		IWebServer::Headers headers;
		headers["WWW-Authenticate"] = "Basic realm=\"SELF\"";
		a_spRequest->m_spConnection->SendResponse( 401, "Unauthorized", headers, EMPTY_STRING );
		return false;
	}

	return true;
}


void TopicManager::OnStreamRequest(IWebServer::RequestSP a_spRequest)
{
	if (Authenticate(a_spRequest) )
	{
		Log::Debug( "TopicManager", "Received stream request from %s", a_spRequest->m_Origin.c_str() );
		if ( a_spRequest->m_Headers.find("Sec-WebSocket-Key") != a_spRequest->m_Headers.end()
			&& a_spRequest->m_Headers.find("selfId") != a_spRequest->m_Headers.end()
			&& a_spRequest->m_Headers.find("token") != a_spRequest->m_Headers.end())
		{
			const std::string & webSocketKey = a_spRequest->m_Headers["Sec-WebSocket-Key"];
			const std::string & selfId = a_spRequest->m_Headers["selfId"];
			const std::string & token = a_spRequest->m_Headers["token"];
			const std::string & instance = a_spRequest->m_Headers["instance"];
			if ( instance.size() > 0 && m_InstanceId == instance )
			{
				Log::Debug( "TopicManager", "Rejecting circular connection, instanceId %s", instance.c_str() );
				a_spRequest->m_spConnection->SendResponse( 400, 
					"Circular Connection", "Rejecting connection from same instance." );
				return;
			}

			Log::Debug("TopicManager", "Adding new child connection %s", selfId.c_str());
			a_spRequest->m_spConnection->StartWebSocket( webSocketKey );

			Connection::SP spConnection = AddConnection(a_spRequest->m_spConnection);
			spConnection->Authenticate(selfId, token);
		}
		else
		{
			std::string headers;
			for( IWebServer::Headers::const_iterator iHeader = a_spRequest->m_Headers.begin(); 
				iHeader != a_spRequest->m_Headers.end(); ++iHeader )
			{
				headers += iHeader->first + ":" + iHeader->second + "\n";
			}

			Log::Error( "TopicManager", "Stream request is missing headers, headers received: %s", headers.c_str() );
			a_spRequest->m_spConnection->SendResponse(400, "Request Error", "Required headers missing.");
		}
	}
}

void TopicManager::OnWWWRequest(IWebServer::RequestSP a_spRequest)
{
	if (Authenticate(a_spRequest))
	{
		Log::Debug("TopicManager", "Received request for hosted material");

		//! Trim leading or trailing slashes.
		std::string endPoint( a_spRequest->m_EndPoint );

		StringUtil::Replace( endPoint, "\\", "/" );	// normalize slashes
		if (StringUtil::EndsWith(endPoint, "/"))
			endPoint = StringUtil::RightTrim(endPoint, "/");

		std::string contentPath = SelfInstance::GetInstance()->GetStaticDataPath() 
			+ "shared" + endPoint;

		// check for index.html in the working directory..
		if ( boost::filesystem::is_directory(contentPath) &&
			boost::filesystem::is_regular_file(contentPath + "/index.html") )
			contentPath += "/index.html";

		if ( boost::filesystem::is_regular_file(contentPath))
		{
			std::ifstream a_File(contentPath.c_str(), std::ios::binary );

			if (a_File.is_open())
			{
				IWebServer::Headers headers;
				std::string content = std::string(std::istreambuf_iterator<char>(a_File), 
					std::istreambuf_iterator<char>());
				
				if ( content.find( '\0' ) != std::string::npos 
					|| StringUtil::EndsWith( contentPath, ".png", true ) 
					|| StringUtil::EndsWith( contentPath, ".jpg", true ) )
				{
					headers["Content-Type"] = "application/octet-stream";
					headers["Content-Length"] = StringUtil::Format( "%u", content.size() );
				}

				a_spRequest->m_spConnection->SendResponse(200, "Connected", headers, content, false);
				a_File.close();
			}
			else
			{
				a_spRequest->m_spConnection->SendResponse(400, "Failed", 
					"Path Invalid: " + contentPath, true);
			}
		}
		else if (boost::filesystem::is_directory(contentPath))
		{
			std::string content;
			content += "<html><body><h1>" + endPoint + "</h1><br><table>";

			size_t lastSlash = endPoint.rfind( "/" );
			if ( lastSlash != std::string::npos )
			{
				std::string parentDirectory( endPoint.substr( 0, lastSlash ) );
				if (! parentDirectory.empty() )
					content += "<tr><td><a href=" + parentDirectory + "/>..</a><br></td></tr>";
			}

			Json::Value directoryStructure;
			directoryStructure["root"] = contentPath;

			//! Build the Json to be returned regardless
			//! If an index.html is found, return the contents of that
			for (boost::filesystem::directory_iterator p(contentPath); 
				p != boost::filesystem::directory_iterator(); ++p)
			{
				std::string fullPath( (*p).path().string() );
				StringUtil::Replace( fullPath, "\\", "/" );		// normalize paths
				std::string fileName( fullPath.substr(fullPath.rfind("/")) );

				content += "<tr><td><a href=" + endPoint + fileName + ">" + fileName + "</a></td></tr>";
			}
			content += "</table></body></html>";

			a_spRequest->m_spConnection->SendResponse(200, "Connected", content, true);
		}
		else
		{
			a_spRequest->m_spConnection->SendResponse(400, "Failed", 
				"Path Invalid: " + contentPath, true );
		}
	}
}

void TopicManager::OnInfoRequest(IWebServer::RequestSP a_spRequest)
{
	if ( Authenticate( a_spRequest ) )
	{
		int index = 0;

		Json::Value json;
		json["selfId"] = m_SelfId;

		SelfInstance * pInstance = SelfInstance::GetInstance();
		if ( pInstance != NULL )
		{
			json["name"] = pInstance->GetLocalConfig().m_Name;
			json["type"] = pInstance->GetRobotType();
		}

		if ( m_TopicMap.size() > 0 )
		{
			Json::Value & topics = json["topics"];
			for( TopicMap::const_iterator iTopic = m_TopicMap.begin(); iTopic != m_TopicMap.end(); ++iTopic )
			{
				Json::Value & topic = topics[index++];
				topic["topicId"] = iTopic->first;
				topic["type"] = iTopic->second.m_Type;
			}
		}

		if ( m_ConnectionMap.size() > 0 )
		{
			index = 0;
			Json::Value & children = json["children"];
			for( ConnectionMap::const_iterator iConn = m_ConnectionMap.begin(); iConn != m_ConnectionMap.end(); ++iConn )
			{
				Json::Value & child = children[index++];
				child["selfId"] = iConn->second->GetSelfId();
				child["authenticated"] = iConn->second->IsAuthenticated();
			}
		}

		if ( m_spParentConnection )
		{
			Json::Value & parent = json["parent"];
			parent["selfId"] = m_spParentConnection->GetSelfId();
			parent["authenticated"] = m_spParentConnection->IsAuthenticated();
		}

		a_spRequest->m_spConnection->SendResponse( 200, "OK", json.toStyledString() );
	}
}

void TopicManager::OnTopicRequest(IWebServer::RequestSP a_spRequest) 
{
	if ( Authenticate( a_spRequest ) )
		AddSubscriber( a_spRequest );
}

void TopicManager::ConnectToParent()
{
	m_spReconnectTimer.reset();

	IWebClient::Headers headers;
	headers["selfId"] = m_SelfId;
	headers["token"] = m_BearerToken;
	headers["instance"] = m_InstanceId;

	m_spWebClient = IWebClient::Create(m_ParentHost + "/stream");
	m_spWebClient->SetHeaders(headers);
	m_spWebClient->SetStateReceiver(DELEGATE(TopicManager, OnClientState, IWebClient *, this));

	Log::Status("TopicManager", "Connecting to parent %s", m_ParentHost.c_str());
	if (!m_spWebClient->Send())
		Log::Error("TopicManager", "Send() return false.");

	m_spParentConnection = Connection::SP( new Connection() );
	m_spParentConnection->Start( this, m_spWebClient );
}

void TopicManager::OnClientState( IWebClient * a_pClient )
{
	Log::Debug("TopicManager", "OnClientState() %u", a_pClient->GetState());
	switch (a_pClient->GetState())
	{
	case IWebClient::CONNECTING:		// trying to establish a connection
	case IWebClient::CONNECTED:			// connection established, ready to send/receive data
	case IWebClient::CLOSING:			// set when Close() is invoked and before it's really closed
		break;
	case IWebClient::CLOSED:			// connection has been closed gracefully
	case IWebClient::DISCONNECTED:		// connection has been lost
	{
		Log::Debug("TopicManager", "Connection to parent closed/disconnected.");
		if (m_bActive)
		{
			if (TimerPool::Instance() != NULL)
			{
				Log::Status("TopicManager", "Disconnected -- Will try to connect to parent in %f seconds.", m_fReconnectInterval);
				m_spReconnectTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(TopicManager, ConnectToParent, this), m_fReconnectInterval, true, false);
			}
			else
				Log::Warning("TopicManager", "No TimerPool, unable to start automatic reconnect.");

		}
		m_spParentConnection.reset();
		break;
	}
	}
}

TopicManager::Subscriber::SP TopicManager::AddSubscriber(IWebServer::RequestSP a_spRequest)
{
	Subscriber::SP spSub( new Subscriber() );
	if ( spSub->Start( this, a_spRequest ) )
		m_SubscriberMap[ spSub->GetSubscriberId() ] = spSub;
	Log::Status( "TopicManager", "Added subscriber %u", spSub->GetSubscriberId() );
	return spSub;
}

void TopicManager::RemoveSubscriber(const Subscriber::SP & a_spSubscriber)
{
	Log::Status( "TopicManager", "Removing subscriber %u", a_spSubscriber->GetSubscriberId() );
	m_SubscriberMap.erase(a_spSubscriber->GetSubscriberId());
}

TopicManager::Connection::SP TopicManager::AddConnection( IWebSocket::SP a_spSocket )
{
	Connection::SP spConnection( new Connection() );
	spConnection->Start( this, a_spSocket );
	m_ConnectionMap[ spConnection->GetConnectionId() ] = spConnection;

	Log::Status( "TopicManager", "Added connection %u", 
		spConnection->GetConnectionId() );
	return spConnection;
}

void TopicManager::RemoveConnection(const Connection::SP & a_spConnection)
{
	std::string origin( a_spConnection->GetSelfId() + "/" );

	Log::Status( "TopicManager", "Removing connection %u, SelfId: %s", 
		a_spConnection->GetConnectionId(), a_spConnection->GetSelfId().c_str() );

	Json::Value removing;
	removing["event"] = "disconnected";
	removing["selfId"] = a_spConnection->GetSelfId();
	Publish( "topic-manager", removing.toStyledString() );

	// remove all subscribers through this connection.
	for( TopicMap::iterator iTopic = m_TopicMap.begin(); iTopic != m_TopicMap.end(); ++iTopic )
	{
		StringList & subs = iTopic->second.m_Subscribers;
		for (StringList::iterator iSub = subs.begin(); iSub != subs.end(); )
		{
			const std::string & sub = *iSub;
			if ( StringUtil::StartsWith( sub, origin ) )
			{
				Log::Debug("TopicManager", "Unsubcribed %s from topic %s.", 
					sub.c_str(), iTopic->second.m_Topic.c_str());

				if (iTopic->second.m_SubscriberCallback.IsValid())
				{
					SubInfo info;
					info.m_Origin = sub;
					info.m_Subscribed = false;
					info.m_Topic = iTopic->second.m_Topic;

					iTopic->second.m_SubscriberCallback(info);
				}
				subs.erase(iSub++);
			}
			else
			{
				++iSub;
			}
		}
	}

	m_ConnectionMap.erase(a_spConnection->GetConnectionId());
	m_ConnectionLookup.erase(a_spConnection->GetSelfId());

	// do this last, as we might get passed a reference the the m_spParentConnection
	if ( m_spParentConnection == a_spConnection )
		m_spParentConnection.reset();
}

Json::Value TopicManager::GetTargets( const std::string & a_TopicId )
{
	TopicMap::iterator iTopic = m_TopicMap.find(a_TopicId);
	if (iTopic == m_TopicMap.end())
		return false;

	Json::Value targets;

	int index = 0;
	StringList & subscribers = iTopic->second.m_Subscribers;
	for (StringList::iterator iSubscriber = subscribers.begin(); iSubscriber != subscribers.end(); ++iSubscriber)
		targets[index++] = *iSubscriber;

	return targets;
}

TopicManager::Connection::SP TopicManager::ResolveConnection( const std::string & a_Target, std::string & a_Origin )
{
	if (a_Target == ".")
		return Connection::SP();

	if ( a_Target == ".." )
	{
		if (m_spParentConnection)
		{
			a_Origin = m_SelfId + "/" + a_Origin;
			return m_spParentConnection;
		}

		return Connection::SP();
	}

	ConnectionLookup::const_iterator iConn = m_ConnectionLookup.find( a_Target );
	if ( iConn != m_ConnectionLookup.end() )
	{
		a_Origin = "../" + a_Origin;

		ConnectionMap::iterator iConn2 = m_ConnectionMap.find( iConn->second );
		if ( iConn2 != m_ConnectionMap.end() )
			return iConn2->second;
	}

	return Connection::SP();
}

TopicManager::Connection::SP TopicManager::FindConnection( IWebSocket::SP a_spSocket )
{
	if ( m_spParentConnection && m_spParentConnection->GetSocket() == a_spSocket )
		return m_spParentConnection;

	for( ConnectionMap::iterator iConnection = m_ConnectionMap.begin(); iConnection != m_ConnectionMap.end(); ++iConnection )
		if ( iConnection->second->GetSocket() == a_spSocket )
			return iConnection->second;

	return Connection::SP();
}

TopicManager::Connection::SP TopicManager::FindConnection( const std::string & a_SelfId )
{
	if ( m_spParentConnection && m_spParentConnection->GetSelfId() == a_SelfId )
		return m_spParentConnection;
	ConnectionLookup::iterator iConnection = m_ConnectionLookup.find( a_SelfId );
	if ( iConnection != m_ConnectionLookup.end() )
		return m_ConnectionMap[ iConnection->second ];

	return Connection::SP();
}

void TopicManager::RouteMessage( const Json::Value & a_Message, Connection * a_pOrigin /*= NULL*/)
{
	const Json::Value & targets = a_Message["targets"];
	if (targets.size() == 0)
		return;

	// enumerate all targets and consolidate based on the next target, so we 
	// can just send one message to each target.
	TargetMap send;
	for(size_t i=0;i<targets.size();++i)
	{
		const std::string & target = targets[i].asString();

		std::string next, newTarget;
		size_t nSlash = target.find_first_of( '/' );
		if ( nSlash != std::string::npos )
		{
			next = target.substr( 0, nSlash );
			newTarget = target.substr( nSlash + 1 );
		}
		else
		{
			next = ".";
			newTarget = target;
		}

		send[ next ].push_back( newTarget );
	}

	// for each unique target, make a copy of the message then modify the targets/origin of the message
	for(TargetMap::iterator iSend = send.begin(); iSend != send.end(); ++iSend )
	{
		const std::string & target = iSend->first;
		const StringList & targets = iSend->second;

		// make a copy of the message, we will be modifying the target and origin before we send it..
		Json::Value message(a_Message);

		int index = 0;
		Json::Value newTargets;
		for (StringList::const_iterator iTarget = targets.begin(); iTarget != targets.end(); ++iTarget)
			newTargets[index++] = *iTarget;
		message["targets"] = newTargets;

		if (target != "." )
		{
			std::string origin( message["origin"].asString() );
			Connection::SP spConnection = ResolveConnection(target, origin );
			if (spConnection)
			{
				message["origin"] = origin;
				if ( message.isMember("binary") && message["binary"].asBool() )
				{
					// copy out the data from the json before we try to convert it to text.
					std::string data( message["data"].asString() );
					// replace actual data with the length of the data in bytes
					message["data"] = static_cast<unsigned int>( data.size() );		
					std::string frame( message.toStyledString() );
					// separate binary from header with a single NULL character..
					frame += '\0';		
					// lastly, append the raw binary data..
					frame += data;

					spConnection->GetSocket()->SendBinary(frame);
				}
				else
				{
					spConnection->GetSocket()->SendText(message.toStyledString());
				}
			}
			else
			{
				// no connection resolve, bounce the message back to the origin as a failure.
				Json::Value failure(message);
				failure["msg"] = "no_route";
				failure["targets"].clear();
				failure["targets"][0] = origin;
				failure["origin"] = target;
				failure["failed_msg"] = message["msg"];
				failure["orig_msg"] = message;

				RouteMessage(failure);
			}
		}
		else
		{
			ProcessMessage(message);
		}
	}
}

void TopicManager::ProcessMessage( const Json::Value & a_Message )
{
	const std::string & type = a_Message["msg"].asString();

	MessageHandlerMap::iterator iHandler = m_MessageHandlerMap.find(type);
	if (iHandler != m_MessageHandlerMap.end())
		iHandler->second(a_Message);
	else
		Log::Error("TopicManager", "No message handler: %s", type.c_str());
}

void TopicManager::HandleSubscribe(const Json::Value & a_Message)
{
	std::string origin( a_Message["origin"].asString() );

	const Json::Value & targets = a_Message["targets"];
	for (size_t i = 0; i < targets.size(); ++i)
	{
		std::string topicId( targets[i].asString() );

		TopicMap::iterator iTopic = m_TopicMap.find(topicId);
		if (iTopic != m_TopicMap.end())
		{
			bool bSubscribed = false;
			StringList & subs = iTopic->second.m_Subscribers;
			for (StringList::iterator iSub = subs.begin(); iSub != subs.end() && !bSubscribed; ++iSub)
				if ( *iSub == origin )
					bSubscribed = true;

			if (! bSubscribed )
			{
				SubInfo info;
				info.m_Origin = origin;
				info.m_Subscribed = true;

				Log::Debug("TopicManager", "New subscriber %s to topic %s.", info.m_Origin.c_str(), topicId.c_str());

				iTopic->second.m_Subscribers.push_back(info.m_Origin);
				if (iTopic->second.m_SubscriberCallback.IsValid())
				{
					info.m_Topic = iTopic->second.m_Topic;
					iTopic->second.m_SubscriberCallback(info);
				}

				// if we have persisted data, send it to the new subscriber immediately..
				TopicDataMap::iterator iData = m_TopicDataMap.find(iTopic->second.m_Topic);
				if (iData != m_TopicDataMap.end())
				{
					Log::Debug("TopicManager", "Sending persisted data to %s", info.m_Origin.c_str());

					Json::Value message;
					message["msg"] = "publish";
					message["targets"][0] = info.m_Origin;
					message["origin"] = ".";
					message["data"] = iData->second;
					message["type"] = iTopic->second.m_Type;
					message["persisted"] = true;
					message["topic"] = iTopic->second.m_Topic;

					RouteMessage(message);
				}
			}
			else
			{
				Log::Warning( "TopicManager", "Origin %s is already subscribed to %s", 
					origin.c_str(), topicId.c_str() );
			}
		}
		else
		{
			Log::Warning("TopicManager", "Topic %s not found.", topicId.c_str());

			Json::Value failure(a_Message);
			failure["msg"] = "subscribed_failed";
			failure["targets"].clear();
			failure["targets"][0] = origin;
			failure["origin"] = topicId;
			failure["failed_msg"] = a_Message["msg"];

			RouteMessage(failure);
		}
	}
}

void TopicManager::HandleSubscribeFailed(const Json::Value & a_Message)
{
	Log::Error( "TopicManager", "Failed to subscribe to %s.", a_Message["origin"].asCString() );
}

void TopicManager::HandlePublish(const Json::Value & a_Message)
{
	const std::string & topic = a_Message["topic"].asString();
	const std::string & origin = a_Message["origin"].asString();

	std::string sPath(GetPath(origin, topic));

	SubscriptionMap::iterator iSubs = m_SubscriptionMap.find(sPath);
	if (iSubs != m_SubscriptionMap.end())
	{
		Payload payload;
		payload.m_Topic = topic;
		payload.m_Origin = origin;
		payload.m_Data = a_Message["data"].asString();
		payload.m_Type = a_Message["type"].asString();
		payload.m_Persisted = a_Message["persisted"].asBool();
		if ( a_Message.isMember("remote_origin") )
			payload.m_RemoteOrigin = a_Message["remote_origin"].asString();

		SubscriptionList & subs = iSubs->second;
		for( SubscriptionList::iterator iSub = subs.begin(); iSub != subs.end(); ++iSub )
		{
			Subscription & sub = *iSub;
			if (sub.m_Callback.IsValid())
				sub.m_Callback(payload);
		}
	}
}

void TopicManager::HandlePublishAt(const Json::Value & a_Message)
{
	const Json::Value & targets = a_Message["targets"];
	for (size_t i = 0; i < targets.size(); ++i)
	{
		const std::string & topicId = targets[i].asString();
		const std::string & origin = a_Message["origin"].asString();

		TopicMap::iterator iTopic = m_TopicMap.find(topicId);
		if (iTopic == m_TopicMap.end())
		{
			Log::Warning("TopicManager", "Topic %s not found.", topicId.c_str());
			continue;
		}
	
		Log::Debug("TopicManager", "HandlePublishAt(), origin: %s, topic: %s.", origin.c_str(), topicId.c_str());

		const std::string & data = a_Message["data"].asString();
		bool bPersisted = a_Message["persisted"].asBool();

		if (bPersisted)
			m_TopicDataMap[topicId] = data;

		Json::Value json;
		json["targets"] = GetTargets(topicId);
		json["origin"] = ".";
		json["msg"] = "publish";
		json["data"] = data;
		json["type"] = iTopic->second.m_Type;
		json["persisted"] = bPersisted;
		json["topic"] = topicId;
		json["remote_origin"] = origin;

		RouteMessage(json);
	}
}

void TopicManager::HandleUnsubscribe(const Json::Value & a_Message)
{
	std::string origin = a_Message["origin"].asString();

	const Json::Value & targets = a_Message["targets"];
	for (size_t i = 0; i < targets.size(); ++i)
	{
		const std::string & topicId = targets[i].asString();

		TopicMap::iterator iTopic = m_TopicMap.find(topicId);
		if (iTopic == m_TopicMap.end())
		{
			Log::Warning("TopicManager", "Topic %s not found.", topicId.c_str());
			continue;
		}

		bool bFound = false;

		StringList & subs = iTopic->second.m_Subscribers;
		for (StringList::iterator iSub = subs.begin(); iSub != subs.end(); ++iSub)
		{
			if (*iSub != origin)
				continue;

			bFound = true;
			Log::Debug("TopicManager", "Unsubcribed %s from topic %s.", origin.c_str(), topicId.c_str());

			subs.erase(iSub);
			if (iTopic->second.m_SubscriberCallback.IsValid())
			{
				SubInfo info;
				info.m_Origin = origin;
				info.m_Subscribed = false;
				info.m_Topic = iTopic->second.m_Topic;
				iTopic->second.m_SubscriberCallback(info);
			}
			break;
		}

		if (!bFound)
		{
			Log::Warning("TopicManager", "Failed to unsubscribe from topic %s, target %s not found.",
				topicId.c_str(), origin.c_str());
		}
	}
}

void TopicManager::HandleNoRoute(const Json::Value & a_Message)
{
	const std::string & origin = a_Message["origin"].asString();

	const Json::Value & targets = a_Message["targets"];
	for (size_t i = 0; i < targets.size(); ++i)
	{
		//const std::string & target = targets[i].asString();
		const std::string & failed_event = a_Message["failed_msg"].asString();

		Log::Debug("TopicManager", "No-Route: %s", a_Message.toStyledString().c_str() );
		if (failed_event == "publish")
		{
			const std::string & topicId = a_Message["topic"].asString();

			// remove the subscriber for this event.
			TopicMap::iterator iTopic = m_TopicMap.find(topicId);
			if (iTopic != m_TopicMap.end())
			{
				StringList & subs = iTopic->second.m_Subscribers;
				for (StringList::iterator iSub = subs.begin(); iSub != subs.end(); ++iSub)
				{
					const std::string & sub = *iSub;
					if (! StringUtil::StartsWith( sub, origin) )
						continue;

					Log::Debug("TopicManager", "Unsubscribed %s from topic %s.", sub.c_str(), topicId.c_str());

					if (iTopic->second.m_SubscriberCallback.IsValid())
					{
						SubInfo info;
						info.m_Origin = sub;
						info.m_Subscribed = false;
						info.m_Topic = iTopic->second.m_Topic;

						iTopic->second.m_SubscriberCallback(info);
					}
					subs.erase(iSub);
					break;
				}
			}
		}
	}
}

void TopicManager::HandleQuery(const Json::Value & a_Message)
{
	std::string origin = a_Message["origin"].asString();
	std::string requestId = a_Message["request"].asString();

	Json::Value resp;
	resp["targets"][0] = origin;
	resp["origin"] = ".";
	resp["msg"] = "query_response";
	resp["request"] = requestId;
	resp["selfId"] = m_SelfId;

	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance != NULL )
	{
		resp["name"] = pInstance->GetLocalConfig().m_Name;
		resp["type"] = pInstance->GetRobotType();
		resp["version"] = pInstance->GetSelfVersion();
	}

	if (m_spParentConnection)
		resp["parentId"] = m_spParentConnection->GetSelfId();

	int index = 0;
	for (ConnectionMap::iterator iConn = m_ConnectionMap.begin(); iConn != m_ConnectionMap.end(); ++iConn)
		resp["children"][index++] = iConn->second->GetSelfId();

	index = 0;
	for (TopicMap::iterator iTopic = m_TopicMap.begin(); iTopic != m_TopicMap.end(); ++iTopic)
	{
		Json::Value & topic = resp["topics"][index++];
		topic["topicId"] = iTopic->second.m_Topic;
		topic["type"] = iTopic->second.m_Type;
	}

	RouteMessage(resp);
}

void TopicManager::HandleQueryResponse(const Json::Value & a_Message)
{
	unsigned int requestId = strtoul( a_Message["request"].asCString(), 0, 10 );
	QueryRequestMap::iterator iRequest = m_QueryRequestMap.find(requestId);
	if (iRequest != m_QueryRequestMap.end() && iRequest->second.m_Callback.IsValid() )
	{
		QueryInfo info;
		info.m_bSuccess = true;
		info.m_Path = iRequest->second.m_Path;
		info.m_SelfId = a_Message["selfId"].asString();
		info.m_Name = a_Message["name"].asString();
		info.m_Type = a_Message["type"].asString();

		if (a_Message.isMember("parentId"))
			info.m_ParentId = a_Message["parentId"].asString();
		if (a_Message.isMember("children"))
		{
			const Json::Value & children = a_Message["children"];
			for (size_t i = 0; i < children.size(); ++i)
				info.m_Children.push_back(children[i].asString());
		}
		if (a_Message.isMember("topics"))
		{
			const Json::Value & topics = a_Message["topics"];
			for (size_t i = 0; i < topics.size(); ++i)
				info.m_Topics.push_back(TopicInfo(topics[i]["topicId"].asString(), topics[i]["type"].asString()));
		}

		iRequest->second.m_Callback(info);
		m_QueryRequestMap.erase(iRequest);
	}
}

//-----------------------------------------------------------

TopicManager::Connection::Connection() :
	m_pAgent( NULL ),
	m_ConnectionId( 0 ),
	m_bAuthenticated( false )
{}

TopicManager::Connection::~Connection()
{}

void TopicManager::Connection::Start( TopicManager * a_pAgent, IWebSocket::SP a_spSocket )
{
	m_pAgent = a_pAgent;
	m_ConnectionId = m_pAgent->m_NextConnectionId++;
	m_spSocket = a_spSocket;

	a_spSocket->SetFrameReceiver( DELEGATE( Connection, OnFrame, IWebSocket::FrameSP, shared_from_this() ) );
	a_spSocket->SetErrorHandler( DELEGATE(Connection, OnError, IWebSocket *, shared_from_this() ) );

	TimerPool * pTimer = TimerPool::Instance();
	if ( pTimer != NULL )
		m_spKeepAliveTimer = pTimer->StartTimer( VOID_DELEGATE( Connection, OnKeepAlive, this ), PING_INTERVAL, true, true );
}

void TopicManager::Connection::Authenticate( const std::string & a_SelfId, const std::string & a_BearerToken)
{
	m_SelfId = a_SelfId;
	m_BearerToken = a_BearerToken;

	bool bAuthenticating = false;

	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance != NULL )
	{
		IAuthenticate * pService = pInstance->FindService<IAuthenticate>();
		if ( pService != NULL 
			&& pService->IsConfigured()
			&& m_pAgent->m_BearerToken.size() > 0
			&& m_pAgent->m_BearerToken != m_BearerToken )
		{
			Log::Status("TopicManager", "Authenticating connection %u", m_ConnectionId);
			pService->Authenticate( m_BearerToken,
				DELEGATE( Connection, OnAuthenticate, const Json::Value &, shared_from_this() ) );
			bAuthenticating = true;
		}
	}

	if (! bAuthenticating )
	{
		Log::Status( "TopicManager", "Skipping authentication for connection %u", m_ConnectionId );
		OnAuthenticated();
	}
}

void TopicManager::Connection::OnAuthenticate( const Json::Value & a_Response )
{
	bool bFailed = true;
	if ( !a_Response.isNull() && a_Response.isMember( "authenticated" ) )
	{
		bool bAuthenticated = a_Response["authenticated"].asBool();
		if ( bAuthenticated )
		{
			OnAuthenticated();
			bFailed = false;
		}
	}

	if ( bFailed )
	{
		std::string message = a_Response["message"].asString();
		Log::Status( "TopicManager", "Failed to authenticate connection %u, SelfId: %s (%s).",
			m_ConnectionId, m_SelfId.c_str(), message.c_str() );
		
		m_spSocket->SendClose( "Failed to authenticate.");
		m_pAgent->RemoveConnection( shared_from_this() );
	}
}

void TopicManager::Connection::OnAuthenticated()
{
	if (! m_bAuthenticated )
	{
		m_bAuthenticated = true;
		Log::Status( "TopicManager", "Connection %u authenticated, SelfId: %s", 
			m_ConnectionId, m_SelfId.c_str() );

		// check for and remove any previous connection for this self id..
		Connection::SP spPreviousConnection = m_pAgent->FindConnection( m_SelfId );
		if ( spPreviousConnection && spPreviousConnection.get() != this )
		{
			Log::Status("TopicManager", "Removing previous connection %u, SelfId: %s",
				spPreviousConnection->GetConnectionId(), 
				spPreviousConnection->GetSelfId().c_str() );
			m_pAgent->RemoveConnection(spPreviousConnection);
		}

		// update the connection lookup so we can find the connection Id by it's name..
		m_pAgent->m_ConnectionLookup[ m_SelfId ] = m_ConnectionId;		

		Json::Value connected;
		connected["event"] = "connected";
		connected["selfId"] = m_SelfId;
		connected["parent"] = this == m_pAgent->m_spParentConnection.get();

		m_pAgent->Publish( "topic-manager", connected.toStyledString());

		Json::Value json;
		json["control"] = "authenticate";
		json["selfId"] = m_pAgent->m_SelfId;
		json["token"] = m_pAgent->m_BearerToken;

		m_spSocket->SendText(json.toStyledString());

		// send all frames we received while we were authenticating the user..
		FramesList send( m_PendingFrames );
		m_PendingFrames.clear();

		for( FramesList::iterator iFrame = send.begin(); iFrame != send.end(); ++iFrame )
			OnFrame( *iFrame );
	}
}

void TopicManager::Connection::OnFrame( IWebSocket::FrameSP a_spFrame )
{
	if ( m_spSocket == a_spFrame->m_wpSocket.lock() )
	{
		if (a_spFrame->m_Op == IWebSocket::TEXT_FRAME)
		{
			Json::Reader reader(Json::Features::strictMode());

			Json::Value json;
			if (reader.parse(a_spFrame->m_Data, json))
			{
				if (json.isMember("control"))
				{
					const std::string & control = json["control"].asString();
					if (control == "authenticate")
						Authenticate(json["selfId"].asString(), json["token"].asString() );
					else
						Log::Warning("TopicManager", "Unsupported control message: %s", control.c_str());
				}
				else if ( m_bAuthenticated )
				{
					if (json.isMember("msg"))
						m_pAgent->RouteMessage(json, this);
					else
						Log::Warning("TopicManager", "Unknown text frame: %s", a_spFrame->m_Data.c_str());
				}
				else
				{
					m_PendingFrames.push_back( a_spFrame );
					// TODO: Put a limit on how much we push here
				}
			}
			else
				Log::Error("TopicManager", "Failed to parse text frame: %s", a_spFrame->m_Data.c_str());
		}
		else if ( a_spFrame->m_Op == IWebSocket::BINARY_FRAME)
		{
			Json::Reader reader(Json::Features::strictMode());

			const char * pHeaderBegin = a_spFrame->m_Data.c_str();
			const char * pHeaderEnd = pHeaderBegin + strlen( pHeaderBegin );

			Json::Value json;
			if ( reader.parse( pHeaderBegin, pHeaderEnd, json ) )
			{
				size_t dataSize = json["data"].asUInt();
				std::string data( pHeaderEnd + 1, dataSize );
				json["data"] = data;

				if ( m_bAuthenticated )
				{
					if (json.isMember("msg"))
						m_pAgent->RouteMessage(json, this);
					else
						Log::Warning("TopicManager", "Unknown binary frame: %s", a_spFrame->m_Data.c_str());
				}
				else
				{
					m_PendingFrames.push_back( a_spFrame );
				}
			}
		}
		else if ( a_spFrame->m_Op == IWebSocket::CLOSE )
		{
			Log::Status("TopicManager", "Close op received (%s), removing connection %u, SelfId: %s", 
				a_spFrame->m_Data.c_str(), GetConnectionId(), GetSelfId().c_str() );
			m_pAgent->RemoveConnection(shared_from_this());
		}
		else if ( a_spFrame->m_Op == IWebSocket::PING )
		{
			Log::DebugLow( "TopicManager", "Receiving ping, sending pong." );
			m_spSocket->SendPong( a_spFrame->m_Data );
		}
		else if ( a_spFrame->m_Op == IWebSocket::PONG )
		{
			double pingTime = atof( a_spFrame->m_Data.c_str() );
			Log::DebugLow( "TopicManager", "Received pong, round trip time %g", Time().GetEpochTime()-  pingTime );
		}
	}
}

void TopicManager::Connection::OnError( IWebSocket * )
{
	Log::Error("TopicManager", "Error on connection %u, SelfId: %s, removing...", 
		GetConnectionId(), GetSelfId().c_str() );
	m_pAgent->RemoveConnection( shared_from_this() );
}

void TopicManager::Connection::OnKeepAlive()
{
	m_spSocket->SendPing( StringUtil::Format( "%f", Time().GetEpochTime() ) );
}

//----------------------------------------------------------------------------

TopicManager::Subscriber::Subscriber() :
	m_pManager( NULL ),
	m_nSubscriberId( 0 ),
	m_bHeaderSent( false ),
	m_ContentLen( 0 )
{}

TopicManager::Subscriber::~Subscriber()
{
	Stop();
}

bool TopicManager::Subscriber::Start( TopicManager * a_pManager, const IWebServer::RequestSP & a_spRequest )
{
	m_pManager = a_pManager;
	m_nSubscriberId = m_pManager->m_NextSubscriberId++;
	m_spRequest = a_spRequest;

	a_spRequest->m_spConnection->SetErrorHandler( DELEGATE(Subscriber, OnError, IWebSocket *, shared_from_this() ) );

	const std::string TOPICS( "/topics/");
	size_t nTopics = a_spRequest->m_EndPoint.find( TOPICS );
	if ( nTopics != std::string::npos )
		m_TopicId = a_spRequest->m_EndPoint.substr( nTopics + TOPICS.size() );

	if ( m_spRequest->m_RequestType == "GET" ) 
	{
		Log::Status( "TopicManager", "Added remote subscriber to topic %s", m_TopicId.c_str() );
		if ( m_pManager->Subscribe( m_TopicId, DELEGATE( Subscriber, OnPayload, const ITopics::Payload &, shared_from_this() ) ) )
			return true;
	}
	else if ( m_spRequest->m_RequestType == "POST" || m_spRequest->m_RequestType == "PUT" )
	{
		IWebServer::Headers::const_iterator iContentLen = m_spRequest->m_Headers.find( "Content-Length" );
		if ( iContentLen != m_spRequest->m_Headers.end() )
			m_ContentLen = strtoul( iContentLen->second.c_str(), NULL, 10 );

		if ( m_ContentLen > 0 )
		{
			// send a response so they will send the content..
			m_spRequest->m_spConnection->ReadAsync( 1,
				DELEGATE( Subscriber, OnPublish, std::string *, shared_from_this() ) );
			return true;
		}

		Json::Value error;
		error["error"] = "Content-Length header is missing.";
		m_spRequest->m_spConnection->SendResponse( 500, "ERROR",  error.toStyledString() );
	}
	else
		Log::Error( "TopicManager", "Unsupported Request type %s", m_spRequest->m_RequestType.c_str() );

	Json::Value error;
	error["error"] = StringUtil::Format( "Failed to subscribe to topic: %s", m_TopicId.c_str() );
	m_spRequest->m_spConnection->SendResponse( 500, "TOPIC_NOT_FOUND", error.toStyledString() );
	return false;
}

void TopicManager::Subscriber::Stop()
{
	m_pManager->Unsubscribe( m_TopicId, this );
}

void TopicManager::Subscriber::OnPublish( std::string * a_Data )
{
	m_Publish += *a_Data;
	delete a_Data;

	if ( m_Publish.size() >= m_ContentLen )
	{
		bool bPersistData = StringUtil::Compare( m_spRequest->m_Headers[ "Data-Persisted" ], "YES", true ) == 0;
		bool bBinaryData = StringUtil::Compare( m_spRequest->m_Headers[ "Data-Binary" ], "YES", true ) == 0;

		if ( !m_pManager->PublishAt( m_TopicId, m_Publish, bPersistData, bBinaryData ) )
		{
			Log::Error( "TopicManager", "Failed to publish to %s", m_TopicId.c_str() );
			m_spRequest->m_spConnection->SendResponse( 500, "ERROR", EMPTY_STRING );
		}
		else
		{
			Log::Status( "TopicManager", "Remote subscriber published to topic %s", m_TopicId.c_str() );
			m_spRequest->m_spConnection->SendResponse( 200, "OK", EMPTY_STRING );
		}
	}
	else
	{
		m_spRequest->m_spConnection->ReadAsync( 1,
			DELEGATE( Subscriber, OnPublish, std::string *, shared_from_this() ) );
	}
}

void TopicManager::Subscriber::OnPayload( const ITopics::Payload & a_Payload )
{
	if (! m_bHeaderSent )
	{
		m_bHeaderSent = true;

		IWebServer::Headers headers;
		if ( a_Payload.m_Type == "image/jpeg" || a_Payload.m_Type == "image/png" )
		{
			m_Boundry = UniqueID().Get();
			headers["Content-Type"] = "multipart/x-mixed-replace;boundary=" + m_Boundry;
		}
		else if ( a_Payload.m_Type == "application/json" || a_Payload.m_Type == "text/plain" )
			headers["Content-Type"] = "text/event-stream";
		else
			headers["Content-Type"] = a_Payload.m_Type;

		headers["Cache-Control"] = "no-cache";
		headers["Access-Control-Allow-Origin"] = "*";
		headers["Connection"] = "keep-alive";

		m_spRequest->m_spConnection->SendResponse( 200, "OK", headers, EMPTY_STRING, false );
	}

	std::string content;
	if ( m_Boundry.size() > 0 )
	{
		content += "--" + m_Boundry + "\r\n";
		content += "Content-Type: " + a_Payload.m_Type + "\r\n";
		content += "\r\n";		
	}
	else if ( a_Payload.m_Type == "text/plain" )
		content += "\r\n";

	content += a_Payload.m_Data;
	m_spRequest->m_spConnection->SendAsync( content );
}

void TopicManager::Subscriber::OnError( IWebSocket * )
{
	Log::Error( "TopicManager", "Error on connection, removing remote subscriber for topic %s", m_TopicId.c_str() );
	m_pManager->RemoveSubscriber( shared_from_this() );
}

//------------------------------------------------

void TopicManager::OnTopicManagerEvent(const ITopics::Payload & a_Payload)
{
	if (a_Payload.m_RemoteOrigin[0] != 0)
	{
		Json::Reader reader(Json::Features::strictMode());

		Json::Value json;
		if (reader.parse(a_Payload.m_Data, json))
		{
			if (json["event"].isString())
			{
				const std::string & ev = json["event"].asString();
				if (ev == "register_topic")
				{
					const std::string & topicId = json["topicId"].asString();
					const std::string & type = json["type"].asString();

					Log::Status("TopicManager", "Registering topic %s, type: %s",
						topicId.c_str(), type.c_str() );
					RegisterTopic( topicId, type );
				}
				else if (ev == "unregister_topic")
				{
					const std::string & topicId = json["topicId"].asString();
					Log::Status("TopicManager", "Unregistering topic %s",
						topicId.c_str() );
					UnregisterTopic( topicId );
				}
				else if (ev == "error")
				{
					const std::string & failed_event = json["failed_event"].asString();
					Log::Error("TopicManager", "Received error on event: %s", failed_event.c_str());
				}
				else
				{
					Log::Warning("TopicManager", "Received unknown event: %s", json.toStyledString().c_str());
				}
			}
		}
	}
}

