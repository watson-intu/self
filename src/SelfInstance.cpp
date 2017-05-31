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


#define _CRT_SECURE_NO_WARNINGS

#include "SelfInstance.h"
#include "agent/AgentSociety.h"
#include "blackboard/BlackBoard.h"
#include "classifiers/ClassifierManager.h"
#include "extractors/FeatureManager.h"
#include "gestures/GestureManager.h"
#include "gestures/IGesture.h"
#include "sensors/SensorManager.h"
#include "planning/PlanManager.h"
#include "utils/IService.h"
#include "skills/SkillManager.h"
#include "utils/ThreadPool.h"
#include "utils/TimerPool.h"
#include "utils/ServiceConfig.h"
#include "utils/SelfException.h"
#include "utils/IWebClient.h"
#include "topics/TopicManager.h"
#include "services/IGateway.h"
#include "services/IBrowser.h"

#include <fstream>
#include <boost/filesystem.hpp>

const char * BODY_FILE = "shared/bootstrap.json";
const char * PLATFORM_FILE = "shared/platforms/%s/platform.json";
const char * CONFIG_FILE = "config.json";
const double SAVE_INTERVAL = 30.0;

static std::string GetUrlIp(const std::string & a_Url)
{
	const std::string TCP_PREFIX = "tcp://";
	if (a_Url.substr(0, TCP_PREFIX.size()).compare(TCP_PREFIX) == 0)
		return a_Url.substr(TCP_PREFIX.size());
	return a_Url;
}

RTTI_IMPL(SelfInstance, Config);
RTTI_IMPL_EMBEDDED(SelfInstance, LocalConfig, ISerializable );

SelfInstance * SelfInstance::sm_pInstance = NULL;

SelfInstance::SelfInstance( const std::string & a_Platform,
	const std::string & a_StaticDataPath /*= ./etc/"*/, 
	const std::string & a_InstanceDataPath /*= "./"*/ ) : 
	Config( a_StaticDataPath, a_InstanceDataPath ),
	m_bActive(false),
	m_Platform(a_Platform),
	m_pTopicManager(new TopicManager()),
	m_pTopics(m_pTopicManager),
	m_pBlackBoard(new BlackBoard()),
	m_pGestureManager(new GestureManager()),
	m_pSkillManager(new SkillManager()),
	m_pSensorManager(new SensorManager()),
	m_pFeatureManager(new FeatureManager()),
	m_pClassifierManager(new ClassifierManager()),
	m_pPlanManager(new PlanManager()),
	m_pAgentSociety(new AgentSociety()),
	m_pThreadPool(new ThreadPool()),
	m_pTimerPool(new TimerPool()),
	m_InstanceId( UniqueID().Get() ),
	m_BackendLanguage("en-US"),
	m_LaunchUrl("http://localhost:9443/www/dashboard"),
	m_LogoUrl("http://66.media.tumblr.com/292014cfdbec82ae24e672aaf2fb41c8/tumblr_o6k2spJ4H01s8c9jeo1_400.gif"),
	m_fDefaultVolume(70.0f),
	m_bSaveOnStop(true),
	m_KnowledgeGraphId("knowledge"),
	m_KnowledgeGraphType("SelfGraph"),
	m_KnowledgeGraphServiceId("GraphV1"),
	m_bKnowledgeGraphReady(false)
{
	if (sm_pInstance != NULL)
		throw SelfException("SelfInstance already instantiated.");
	sm_pInstance = this;

	// load in the instance data into this object as well..
	std::string configFile(m_InstanceDataPath + CONFIG_FILE);
	if (ISerializable::DeserializeFromFile( configFile, &m_LocalConfig ) == NULL )
		throw SelfException( "Failed to load config.json" );
	else
		Log::Status( "SelfInstance", "Loaded config: %s", configFile.c_str() );

	if ( m_LocalConfig.m_BodyId.empty() )
	{
		m_LocalConfig.m_BodyId = UniqueID().Get();
		Log::Status( "SelfInstance", "Generating new body ID: %s",
			m_LocalConfig.m_BodyId.c_str() );
	}
}

SelfInstance::~SelfInstance()
{
	Stop();

	delete m_pBlackBoard;
	delete m_pSkillManager;
	delete m_pGestureManager;
	delete m_pSensorManager;
	delete m_pClassifierManager;
	delete m_pPlanManager;
	delete m_pFeatureManager;
	delete m_pAgentSociety;
	delete m_pTimerPool;
	delete m_pThreadPool;

	if (sm_pInstance == this)
		sm_pInstance = NULL;
}

SelfInstance * SelfInstance::GetInstance()
{
	return sm_pInstance;
}

void SelfInstance::Serialize(Json::Value & json)
{
	Config::Serialize(json);

	json["m_KnowledgeGraphId"] = m_KnowledgeGraphId;
	json["m_KnowledgeGraphType"] = m_KnowledgeGraphType;
	json["m_KnowledgeGraphServiceId"] = m_KnowledgeGraphServiceId;
	json["m_RobotType"] = m_RobotType;
	json["m_BackendLanguage"] = m_BackendLanguage;
	json["m_SelfVersion"] = m_SelfVersion;
	json["m_LaunchUrl"] = m_LaunchUrl;
	json["m_LogoUrl"] = m_LogoUrl;
	json["m_fDefaultVolume"] = m_fDefaultVolume;

	SerializeList("m_Agents", m_Agents, json);
	SerializeList("m_Sensors", m_Sensors, json);
	SerializeList("m_Extractors", m_Extractors, json);
	SerializeList("m_Classifiers", m_Classifiers, json);
	SerializeList("m_GestureFiles", m_GestureFiles, json);
	SerializeList("m_SkillFiles", m_SkillFiles, json);
	SerializeList("m_PlanFiles", m_PlanFiles, json);
	SerializeList("m_Interfaces", m_Interfaces, json);

	json["m_pTopicManager"] = ISerializable::SerializeObject(m_pTopicManager);
}

void SelfInstance::Deserialize(const Json::Value & json)
{
	ReleaseObjects();

	Config::Deserialize(json);

	if (json["m_KnowledgeGraphId"].isString())
		m_KnowledgeGraphId = json["m_KnowledgeGraphId"].asString();
	if (json["m_KnowledgeGraphType"].isString())
		m_KnowledgeGraphType = json["m_KnowledgeGraphType"].asString();
	if (json["m_KnowledgeGraphServiceId"].isString())
		m_KnowledgeGraphServiceId = json["m_KnowledgeGraphServiceId"].asString();
	if (json["m_RobotType"].isString())
		m_RobotType = json["m_RobotType"].asString();
	if (json["m_BackendLanguage"].isString())
		m_BackendLanguage = json["m_BackendLanguage"].asString();
	if (json["m_SelfVersion"].isString())
		m_SelfVersion = json["m_SelfVersion"].asString();
	if (json["m_LaunchUrl"].isString())
		m_LaunchUrl = json["m_LaunchUrl"].asString();
	if (json["m_LogoUrl"].isString())
		m_LogoUrl = json["m_LogoUrl"].asString();
	if (json["m_fDefaultVolume"].isNumeric())
		m_fDefaultVolume = json["m_fDefaultVolume"].asFloat();
	if (json["m_pTopicManager"].isObject())
		ISerializable::DeserializeObject(json["m_pTopicManager"], m_pTopicManager);

	DeserializeList("m_Agents", json, m_Agents);
	DeserializeList("m_Sensors", json, m_Sensors);
	DeserializeList("m_Extractors", json, m_Extractors);
	DeserializeList("m_Classifiers", json, m_Classifiers);
	DeserializeList("m_GestureFiles", json, m_GestureFiles);
	DeserializeList("m_SkillFiles", json, m_SkillFiles);
	DeserializeList("m_PlanFiles", json, m_PlanFiles);
	DeserializeList("m_Interfaces", json, m_Interfaces);

	if (m_Interfaces.size() == 0)
		m_Interfaces.push_back("*");
	if (m_GestureFiles.size() == 0)
		m_GestureFiles.push_back("shared/gestures/default.json");
	if (m_SkillFiles.size() == 0)
		m_SkillFiles.push_back("shared/skills/default.json");
	if (m_PlanFiles.size() == 0)
		m_PlanFiles.push_back("shared/plans/default.json");

	if ( m_LocalConfig.m_Name.empty() && json["m_RobotName"].isString())
		m_LocalConfig.m_Name = json["m_RobotName"].asString();
	if (json["m_RobotUrl"].isString())
		m_LocalConfig.m_RobotUrl = json["m_RobotUrl"].asString();
}

bool SelfInstance::DisableLib( const std::string & a_Lib )
{
	// try to remove any objects created from this library..
	for( LoadedLibraryList::iterator iLib = m_LoadedLibs.begin(); iLib != m_LoadedLibs.end(); ++iLib )
	{
		Library * pLib = *iLib;
		if ( pLib->GetLibraryName() == a_Lib )
		{
			const Library::CreatorSet & creators = pLib->GetCreators();
			for( Library::CreatorSet::const_iterator iCreator = creators.begin();
				iCreator != creators.end(); ++iCreator )
			{
				ICreator * pCreator = *iCreator;

				std::list<IWidget *> remove;

				// enumerate all the objects, add them to a separate list for removal..
				const ICreator::ObjectSet & objects = pCreator->GetObjects();
				for( ICreator::ObjectSet::const_iterator iObject = objects.begin();
					iObject != objects.end(); ++iObject )
				{
					remove.push_back( *iObject );
				}

				// now actually go remove the objects..
				for( std::list<IWidget *>::iterator iRemove = remove.begin(); 
					iRemove != remove.end(); ++iRemove )
				{
					bool bRemoved = false;

					IWidget * pRemove = *iRemove;
					ISerializable * pObject = DynamicCast<ISerializable>( pRemove );
					if ( pObject != NULL )
						bRemoved = RemoveObject( pObject );

					if (! bRemoved )
					{
						Log::Warning( "SelfInstamce", "Unable to remove object %p (%s)", 
							pRemove, pRemove->GetRTTI().GetName().c_str() );
					}
				}
			}
		}
	}

	return Config::DisableLib( a_Lib );
}

//! Initialize this self instance.
bool SelfInstance::Start()
{
	Log::Status("SelfInstance", "Starting SelfInstance");
	if (m_bActive)
	{
		Log::Status("SelfInstance", "SelfInstance already started!");
		return false;		// already started.
	}

	m_bActive = true;
	m_bKnowledgeGraphReady = false;

	m_spKnowledgeGraph = IGraph::SP( IGraph::Create( m_KnowledgeGraphType, m_KnowledgeGraphId) );
	if (! m_spKnowledgeGraph->Load( DELEGATE( SelfInstance, OnKnowledgeGraphLoaded, IGraph::SP, this ) ) )
	{
		Log::Error("SelfInstance", "Failed to load knowledge graph." );
		return false;
	}

	// spin until knowledge graph is loaded..
	while( !m_bKnowledgeGraphReady )
	{
		ThreadPool::Instance()->ProcessMainThread();
		boost::this_thread::yield();
	}

	// load our body from the models of self, this will block until the body is loaded..
	LoadBody();

	if (! StartServices() )
	{
		Log::Error( "SelfInstance", "Failed to start services." );
		return false;
	}

	// connect to the remote knowledge graph, this is optional so this is only a warning.
	if (! m_spKnowledgeGraph->Connect( IGraph::OnGraphConnected(), m_KnowledgeGraphServiceId ) )
		Log::Warning("SelfInstance", "Failed to connect knowledge graph." );

	// send any backtraces back to the gateway if found..
	SendBacktraces();

	if (!m_pTopicManager->Start())
	{
		Log::Error("SelfInstance", "Failed to start TopicManager.");
		return false;
	}
	if (!m_pBlackBoard->Start())
	{
		Log::Error("SelfInstance", "Failed to start Blackboard.");
		return false;
	}
	if (!m_pGestureManager->Start())
	{
		Log::Error("SelfInstance", "Failed to start GestureManager.");
		return false;
	}
	if (!m_pSkillManager->Start())
	{
		Log::Error("SelfInstamce", "Failed to start SkillManager.");
		return false;
	}
	if (!m_pPlanManager->Start())
	{
		Log::Error("SelfInstance", "PlanManager failed to start.");
		return false;
	}
	if (!m_pSensorManager->Start())
	{
		Log::Error("SelfInstance", "SensorManager failed to start");
		return false;
	}
	if (!m_pClassifierManager->Start())
	{
		Log::Error("SelfInstance", "ClassifierManager failed to start");
		return false;
	}
	if (!m_pFeatureManager->Start())
	{
		Log::Error("SelfInstance", "FeatureManager failed to start");
		return false;
	}
	if (!m_pAgentSociety->Start())
	{
		Log::Error("SelfInstance", "AgentSociety failed to start");
		return false;
	}

	// TODO: Move this into NaoPlatform and have it just set the volume of the robot directly 
	// on startup.
	Log::DebugLow("SelfInstance", "Setting default volume to: %f", m_fDefaultVolume);
	m_pSkillManager->UseSkill("volume_default", ParamsMap("m_fTargetVolume", m_fDefaultVolume));

	m_pTopicManager->RegisterTopic("config", "application/json" );
	m_pTopicManager->Subscribe( "config", DELEGATE( SelfInstance, OnConfigEvent, const ITopics::Payload &, this ) );

	RegisterEventHandlers();
		
	TimerPool * pTimers = TimerPool::Instance();
	if (pTimers != NULL)
		m_spSaveInstanceData = pTimers->StartTimer(VOID_DELEGATE(SelfInstance, OnSaveConfig, this), SAVE_INTERVAL, true, true);

	OnSaveConfig();

	if (! m_LaunchUrl.empty() )
	{
		IBrowser * pBrowser = FindService<IBrowser>();
		if ( pBrowser != NULL )
		{
			Log::Status( "SelfInstance", "Launching Url: %s", m_LaunchUrl.c_str() );
			pBrowser->ShowURL( Url::SP( new Url( m_LaunchUrl ) ), IBrowser::UrlCallback() );
		}
	}

	Log::Status("SelfInstance", "Started SelfInstance");
	return true;
}

bool SelfInstance::Update()
{
	m_pThreadPool->ProcessMainThread();
	return true;
}

int SelfInstance::Run()
{
	if (!Start())
		return false;

	int exitCode = m_pThreadPool->RunMainThread();

	Stop();

	return exitCode;
}

bool SelfInstance::Stop()
{
	if (!m_bActive)
		return false;

	m_spSaveInstanceData.reset();

	m_bActive = false;

	if (!m_pAgentSociety->Stop())
		Log::Error("SelfInstance", "Failed to stop AgentSociety.");
	if (!m_pFeatureManager->Stop())
		Log::Error("SelfInstance", "Failed to stop FeatureManager.");
	if (!m_pClassifierManager->Stop())
		Log::Error("SelfInstance", "Failed to stop ClassifierManager.");

	// stop all the services, this may block until all asynchronous requests are done..
	if (! StopServices() )
		Log::Error("SelfInstance", "Failed to stop services." );
	if (!m_pPlanManager->Stop())
		Log::Error("SelfInstamce", "Failed to stop PlanManager.");
	if (!m_pSensorManager->Stop())
		Log::Error("SelfInstance", "Failed to stop SensorManager.");
	if (!m_pSkillManager->Stop())
		Log::Error("SelfInstance", "Failed to stop SKillManager.");
	if (!m_pGestureManager->Stop())
		Log::Error("SelfInstance", "Failed to stop GestureManager.");
	if (!m_pBlackBoard->Stop())
		Log::Error("SelfInstance", "Failed to stop Blackboard.");
	if (!m_pTopicManager->Stop())
		Log::Error("SelfInstance", "Failed to stop TopicManager.");

	// save body state back out to storage.
	if (m_bSaveOnStop)
		OnSaveConfig();

	if ( m_spKnowledgeGraph )
	{
		m_spKnowledgeGraph->Close();
		m_spKnowledgeGraph.reset();
	}

	Log::Status("SelfInstance", "Self Stopped");
	return true;
}

void SelfInstance::ReleaseObjects()
{
	m_Sensors.clear();
	m_Agents.clear();
	m_Extractors.clear();
	m_Classifiers.clear();
}

void SelfInstance::SendBacktraces()
{
	if ( boost::filesystem::exists( "backtrace.log" ) )
	{
		std::ifstream input( "backtrace.log" );
		if (input.is_open())
		{
			std::string bt = std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
			input.close();

			IGateway * pGateway = FindService<IGateway>();
			if ( pGateway != NULL )
			{
				Log::Status( "SelfInstance", "Sending backtrace.log to gateway." );
				pGateway->SendBacktrace( bt );
			}
			else
				Log::Error( "SelfInstance", "No gateway service found, cannot send backtrace.log" );
		}

		// remove the file..
		try {
			boost::filesystem::remove( "backtrace.log" );
		}
		catch( const std::exception &  )
		{}
	}
}

void SelfInstance::ApplyLocalConfig()
{
	// apply the gateway URL
	ServiceConfig * pGateway = FindServiceConfig( "RobotGatewayV1" );
	if ( pGateway != NULL )
	{
		if ( m_LocalConfig.m_GatewayURL.size() > 0 )
			pGateway->m_URL = m_LocalConfig.m_GatewayURL;
		else
			m_LocalConfig.m_GatewayURL = pGateway->m_URL;
	}

	// set the port
	if ( m_LocalConfig.m_nPort > 0 )
		m_pTopicManager->SetPort( m_LocalConfig.m_nPort );

	// grab the latest mac address and store this in the body.
	if ( m_LocalConfig.m_MacId.size() == 0 )
	{
		const char * pMAC = getenv( "MAC_ID" );
		if ( pMAC != NULL )
			m_LocalConfig.m_MacId = pMAC;

		if ( m_LocalConfig.m_MacId.size() == 0 )
			m_LocalConfig.m_MacId = GetMac::GetMyAddress(m_Interfaces);
	}
	m_LocalConfig.m_Ip = GetMac::GetIpAddress();

	// set the clientId
	IWebClient::SetClientId( m_LocalConfig.m_MacId );
}

void SelfInstance::OnSaveConfig()
{
	// save body state back out to storage.
	std::string configFile(m_InstanceDataPath + CONFIG_FILE);
	if (!ISerializable::SerializeToFile(configFile, &m_LocalConfig, true, true))
		Log::Error("SelfInstamce", "Failed to save local config to %s.", configFile.c_str());
	else
		Log::Status("SelfInstance", "Local config saved to %s.", configFile.c_str());
}

void SelfInstance::SetBearerToken(const std::string & a_BearerToken)
{
	m_LocalConfig.m_BearerToken = a_BearerToken;
	if ( m_pTopicManager != NULL )
		m_pTopicManager->SetBearerToken( a_BearerToken );
}

void SelfInstance::SetEmbodimentId(const std::string & a_EmbodimentId)
{
	m_LocalConfig.m_EmbodimentId = a_EmbodimentId;
	if ( m_pTopicManager != NULL )
		m_pTopicManager->SetSelfId( a_EmbodimentId );
}

void SelfInstance::OnKnowledgeGraphLoaded( IGraph::SP a_spGraph )
{
	assert( a_spGraph == m_spKnowledgeGraph );
	m_bKnowledgeGraphReady = true;
}

void SelfInstance::LoadBody()
{
	assert(! m_LocalConfig.m_BodyId.empty() );

	m_spMyBody.reset();

	m_spKnowledgeGraph->SetModel( "self" );
	ITraverser::SP spLoadBody = m_spKnowledgeGraph->CreateTraverser(
		LogicalCondition( Logic::AND,
			LabelCondition( "body" ), 
			EqualityCondition( "bodyId", Logic::EQ, m_LocalConfig.m_BodyId ) 
		) );

	if ( ! spLoadBody->Start( DELEGATE( SelfInstance, OnBodyLoaded, ITraverser::SP, this ) ) )
		Log::Error( "SelfGraph", "Failed to discover model." );

	while(! m_spMyBody )
	{
		boost::this_thread::yield();
		ThreadPool::Instance()->ProcessMainThread();
	}
}

void SelfInstance::SaveBody()
{
	m_spKnowledgeGraph->SetModel( "self" );

	IGraph::PropertyMap props;
	props["bodyId"] = m_LocalConfig.m_BodyId;
	props["data"] = ISerializable::SerializeObject( this ).toStyledString();

	if (! m_spMyBody )
	{
		Log::Status( "SelfInstance", "Creating new body vertex for body ID %s", 
			m_LocalConfig.m_BodyId.c_str() );

		m_spMyBody = m_spKnowledgeGraph->CreateVertex( "body", props );
	}
	else
	{
		Log::Status( "SelfInstance", "Saving body vertex for body ID %s",
			m_LocalConfig.m_BodyId.c_str() );

		m_spMyBody->SetProperties( props );
		m_spMyBody->Save();
	}
}

void SelfInstance::OnBodyLoaded(ITraverser::SP a_spTraverser)
{
	if ( a_spTraverser->Size() > 0 )
	{
		m_spMyBody = a_spTraverser->GetResult(0);

		std::string json( (*m_spMyBody)["data"].asString() );
		if (! ISerializable::DeserializeObject( json, this ) )
			Log::Error( "SelfInstance", "Failed to deserialize body: %s", json.c_str() );

		ApplyLocalConfig();
	}
	else
	{
		// load the bootstrap.json & platform.json, merge them together..
		Json::Value platform;
		std::string platformFile( m_StaticDataPath + StringUtil::Format(PLATFORM_FILE,m_Platform.c_str()) );
		if (! JsonHelpers::Load( platformFile, platform) )
			throw SelfException( "Failed to load platform.json");
		else
			Log::Status( "SelfInstance", "Loaded platform: %s", platformFile.c_str() );

		Json::Value body;
		std::string bodyFile( m_StaticDataPath + BODY_FILE );
		if (! JsonHelpers::Load( bodyFile, body ) )
			throw SelfException( "Failed to load body" );
		else
			Log::Status( "SelfInstance", "Loaded profile: %s", bodyFile.c_str() );
		JsonHelpers::Merge( platform, body );

		// load the body up, this describes the local system and all the agents, sensors, and services
		// we should initialize.
		if (ISerializable::DeserializeObject( platform, this ) == NULL)
			throw SelfException( "Failed to load body.json.");

		ApplyLocalConfig();
		SaveBody();
	}
}

void SelfInstance::RegisterEventHandlers()
{
	m_EventHandlers["log_level"] = DELEGATE( SelfInstance, OnLogLevel, const Event &, this );
	m_EventHandlers["get_config"] = DELEGATE( SelfInstance, OnGetConfig, const Event &, this );
	m_EventHandlers["list_libs"] = DELEGATE( SelfInstance, OnListLibraries, const Event &, this );
	m_EventHandlers["list_classes"] = DELEGATE( SelfInstance, OnListClasses, const Event &, this );
	m_EventHandlers["enable_lib"] = DELEGATE( SelfInstance, OnEnableLibrary, const Event &, this );
	m_EventHandlers["disable_lib"] = DELEGATE( SelfInstance, OnDisableLibrary, const Event &, this );
	m_EventHandlers["add_cred"] = DELEGATE( SelfInstance, OnAddCredential, const Event &, this );
	m_EventHandlers["remove_cred"] = DELEGATE( SelfInstance, OnRemoveCredential, const Event &, this );
	m_EventHandlers["add_object"] = DELEGATE( SelfInstance, OnAddObject, const Event &, this );
	m_EventHandlers["remove_object"] = DELEGATE( SelfInstance, OnRemoveObject, const Event &, this );
	m_EventHandlers["config_topic_manager"] = DELEGATE( SelfInstance, OnConfigTopicManager, const Event &, this );
}

void SelfInstance::OnConfigEvent( const ITopics::Payload & a_Payload )
{
	if ( a_Payload.m_RemoteOrigin[0] != 0 )
	{
		Event e( a_Payload );
		if ( e.m_Event["event"].isString() )
		{
			const std::string & ev = e.m_Event["event"].asString();
			
			EventHandlerMap::iterator iHandler = m_EventHandlers.find( ev );
			if ( iHandler != m_EventHandlers.end() )
				iHandler->second( e );
			else
				Log::Warning( "SelfInstance", "Received unknown event: %s", ev.c_str() );
		}
		else
			Log::Warning( "SelfInstance", "Event is missing event field: %s", e.m_Event.toStyledString().c_str() );
	}

}

void SelfInstance::OnLogLevel( const Event & a_Event )
{
	int logLevel = a_Event.m_Event["log_level"].asInt();
	const std::string & logReactor = a_Event.m_Event["log_reactor"].asString();

	Log::Status( "SelfInstance", "Setting log level to %u (%s)", logLevel, logReactor.c_str() );
	boost::lock_guard<boost::recursive_mutex> lock( Log::GetReactorLock() );
	for( Log::ReactorList::const_iterator iReactor = Log::GetReactorList().begin();
		iReactor != Log::GetReactorList().end(); ++iReactor )
	{
		ILogReactor * pReactor = *iReactor;
		if (! logReactor.empty() && logReactor != pReactor->GetRTTI().GetName() )
			continue;
		pReactor->SetLogLevel( (LogLevel)logLevel );
	}
}

void SelfInstance::OnGetConfig( const Event & a_Event )
{
	Json::Value result;
	result["event"] = "config_sent";
	result["config"] = ISerializable::SerializeObject( this );

	m_pTopicManager->Publish( "config", result.toStyledString() );
}

static void EnumClasses( RTTI * a_pType, Json::Value & a_Classes )
{
	if ( a_pType != NULL )
	{
		const RTTI::ClassList & children = a_pType->GetChildClasses();
		for( RTTI::ClassList::const_iterator iChild = children.begin(); 
			iChild != children.end(); ++iChild)
		{
			a_Classes["classes"].append( (*iChild)->GetName() );
			EnumClasses( *iChild, a_Classes );
		}
	}
}

void SelfInstance::OnListClasses( const Event & a_Event )
{
	Json::Value result( a_Event.m_Event );
	result["event"] = "class_list";

	bool bSuccess = false;

	const std::string & base_class = a_Event.m_Event["base_class"].asString();
	if (! base_class.empty() )
	{
		RTTI * pRTTI = RTTI::FindType( base_class );
		if ( pRTTI != NULL )
		{
			bSuccess = true;
			EnumClasses( pRTTI, result );
		}
	}

	result["success"] = bSuccess;
	m_pTopicManager->Publish( "config", result.toStyledString() );
}

void SelfInstance::OnListLibraries( const Event & a_Event )
{
	Json::Value result( a_Event.m_Event );
	result["event"] = "libs_list";

	for(LibraryList::iterator iLib = m_Libs.begin();iLib != m_Libs.end(); ++iLib )
	{
		Json::Value lib;
		lib["name"] = *iLib;
		lib["disabled"] = std::find( m_DisabledLibs.begin(), m_DisabledLibs.end(), *iLib ) != m_DisabledLibs.end();

		result["libs"].append( lib );
	}

	result["success"] = true;
	m_pTopicManager->Publish( "config", result.toStyledString() );
}

void SelfInstance::OnEnableLibrary( const Event & a_Event )
{
	Json::Value result( a_Event.m_Event );
	result["event"] = "lib_enabled";

	const std::string & lib = a_Event.m_Event["lib"].asString();
	bool bSuccess = EnableLib( lib );

	if (! bSuccess )
		Log::Warning( "SelfInstance", "Failed to enable library %s", lib.c_str() );

	result["success"] = bSuccess;
	m_pTopicManager->Publish( "config", result.toStyledString() );

	SaveBody();
}

void SelfInstance::OnDisableLibrary( const Event & a_Event )
{
	Json::Value result( a_Event.m_Event );
	result["event"] = "lib_disabled";

	const std::string & lib = a_Event.m_Event["lib"].asString();
	bool bSuccess = DisableLib( lib );

	if (! bSuccess )
		Log::Warning( "SelfInstance", "Failed to disable library %s", lib.c_str() );

	result["success"] = bSuccess;
	m_pTopicManager->Publish( "config", result.toStyledString() );

	SaveBody();
}

void SelfInstance::OnAddCredential( const Event & a_Event )
{
	Json::Value result( a_Event.m_Event );
	result["event"] = "cred_added";

	bool bSuccess = false;

	ServiceConfig config;
	if ( ISerializable::DeserializeObject( a_Event.m_Event["config"], &config ) != NULL )
		bSuccess = AddServiceConfig( config );

	if ( bSuccess )
		Log::Status( "SelfInstamce", "Credential added." );
	else
		Log::Warning( "SelfInstamce", "Failed to add credential." );

	result["success"] = bSuccess;
	m_pTopicManager->Publish( "config", result.toStyledString() );

	SaveBody();
}

void SelfInstance::OnRemoveCredential( const Event & a_Event )
{
	Json::Value result( a_Event.m_Event );
	result["event"] = "cred_removed";

	const std::string & serviceId = a_Event.m_Event["serviceId"].asString();
	bool bSuccess = RemoveServiceConfig( serviceId );

	if ( bSuccess )
		Log::Status( "SelfInstamce", "Credential removed." );
	else
		Log::Warning( "SelfInstamce", "Failed to remove credential." );

	result["success"] = bSuccess;
	m_pTopicManager->Publish( "config", result.toStyledString() );

	SaveBody();
}

void SelfInstance::OnAddObject( const Event & a_Event )
{
	Json::Value result( a_Event.m_Event );
	result["event"] = "object_added";

	bool bSuccess = false;

	const Json::Value & data = a_Event.m_Event["object"];
	bool bOverride = a_Event.m_Event["override"].asBool();
	if (! data.isNull() )
	{
		bool bAddObject = true;

		const std::string & guid = data["GUID_"].asString();
		if (! guid.empty() )
		{
			// look for any existing object
			ISerializable * pPrevObject = DynamicCast<ISerializable>( IWidget::FindWidget( guid ) );
			if ( pPrevObject != NULL )
			{
				std::string name( pPrevObject->GetRTTI().GetName() );
				if ( RemoveObject( pPrevObject ) )
				{
					Log::Status( "SelfInstance", "Removed previous object %s (%s).", 
						name.c_str(), guid.c_str() );
				}
				else
				{
					bAddObject = false;
					Log::Warning( "SelfInstance", "Failed to remove previous object %s (%s).",
						name.c_str(), guid.c_str() );
				}
			}
		}

		if ( bAddObject )
		{
			// make the new object
			ISerializable * pObject = ISerializable::DeserializeObject( data );
			if ( pObject != NULL )
			{
				if ( AddObject( pObject, bOverride ) )
				{
					bSuccess = true;

					Log::Status( "SelfInstance", "Object added %s (%s).", 
						pObject->GetRTTI().GetName().c_str(), pObject->GetGUID().c_str() );
					// send back the full configuration of the object, this is done because
					// the user may not have provided a full object
					result["object"] = ISerializable::SerializeObject( pObject );
				}
				else
				{
					delete pObject;
					Log::Warning( "SelfInstance", "Failed to add object %s", pObject->GetRTTI().GetName().c_str() );
				}
			}
			else
				Log::Warning( "SelfInstamce", "Failed to create object: %s", data.toStyledString().c_str() );
		}
	}
	else
		Log::Warning( "SelfInstance", "object field is missing." );

	result["success"] = bSuccess;
	m_pTopicManager->Publish( "config", result.toStyledString() );

	SaveBody();
}

void SelfInstance::OnRemoveObject( const Event & a_Event )
{
	Json::Value result( a_Event.m_Event );
	result["event"] = "object_removed";

	bool bSuccess = false;
	const std::string & guid = a_Event.m_Event["object_guid"].asString();

	ISerializable * pRemove = DynamicCast<ISerializable>( IWidget::FindWidget( guid ) );
	if ( pRemove != NULL )
	{
		std::string name( pRemove->GetRTTI().GetName() );
		if ( RemoveObject(pRemove) )
		{
			Log::Status( "SelfInstance", "Removed object %s (%s).", 
				name.c_str(), guid.c_str() );
			bSuccess = true;
		}
		else
			Log::Warning( "SelfInstance", "Failed to remove object %s (%s).",
				name.c_str(), guid.c_str() );
	}
	else
		Log::Warning( "SelfInstance", "Failed to find object %s.", guid.c_str() );

	if (! bSuccess )
		Log::Warning( "SelfInstance", "Failed to remove object %s.", guid.c_str() );

	result["success"] = bSuccess;
	m_pTopicManager->Publish( "config", result.toStyledString() );

	SaveBody();
}

void SelfInstance::OnConfigTopicManager( const Event & a_Event )
{
	Json::Value result(a_Event.m_Event);
	const std::string & username = a_Event.m_Event["username"].asString();
	const std::string & password = a_Event.m_Event["password"].asString();
	m_pTopicManager->SetRestUser(username);
	m_pTopicManager->SetRestPassword(password);
	bool bSuccess = true;
	result["success"] = bSuccess;
	m_pTopicManager->Publish("config", result.toStyledString());

	SaveBody();
}

SelfInstance::Event::Event( const ITopics::Payload & a_Payload ) : m_Payload( a_Payload )
{
	Json::Reader reader( Json::Features::strictMode() );
	if (! reader.parse( a_Payload.m_Data, m_Event ) )
		Log::Error( "SelfInstance", "Failed to parse json: %s", reader.getFormattedErrorMessages().c_str() );
}

bool SelfInstance::AddObject( ISerializable * a_pObject, bool a_bOverride )
{
	if ( a_pObject == NULL )
		return false;

	bool bSuccess = false;
	if ( a_pObject->GetRTTI().IsType( &IClassifier::GetStaticRTTI() ) )
	{
		IClassifier::SP spClassifier( (IClassifier *)a_pObject );
		if ( m_pClassifierManager->AddClassifier( spClassifier, a_bOverride ) )
		{
			m_Classifiers.push_back( spClassifier );
			bSuccess = true;
		}
	}
	else if ( a_pObject->GetRTTI().IsType( &IExtractor::GetStaticRTTI() ) )
	{
		IExtractor::SP spExtractor( (IExtractor *)a_pObject );
		if ( m_pFeatureManager->AddFeatureExtractor( spExtractor, a_bOverride ) )
		{
			m_Extractors.push_back( spExtractor );
			bSuccess = true;
		}
	}
	else if ( a_pObject->GetRTTI().IsType( &IAgent::GetStaticRTTI() ) )
	{
		IAgent::SP spAgent( (IAgent *)a_pObject );
		if ( m_pAgentSociety->AddAgent( spAgent, a_bOverride ) )
		{
			m_Agents.push_back( spAgent );
			bSuccess = true;
		}
	}
	else if ( a_pObject->GetRTTI().IsType( &ISensor::GetStaticRTTI() ) )
	{
		ISensor::SP spSensor( (ISensor *)a_pObject );
		if ( m_pSensorManager->AddSensor( spSensor, a_bOverride ) )
		{
			m_Sensors.push_back( spSensor );
			bSuccess = true;
		}
	}
	else if ( a_pObject->GetRTTI().IsType( &IService::GetStaticRTTI() ) )
		bSuccess = AddService( IService::SP( (IService *)a_pObject ) );
	else if ( a_pObject->GetRTTI().IsType( &ISkill::GetStaticRTTI() ) )
		bSuccess = m_pSkillManager->AddSkill( ISkill::SP( static_cast<ISkill *>( a_pObject ) ) );
	else if ( a_pObject->GetRTTI().IsType( &IGesture::GetStaticRTTI() ) )
		bSuccess = m_pGestureManager->AddGesture( IGesture::SP( static_cast<IGesture *>( a_pObject ) ) );
	else if ( a_pObject->GetRTTI().IsType( &Plan::GetStaticRTTI() ) )
		bSuccess = m_pPlanManager->AddPlan( Plan::SP( static_cast<Plan *>( a_pObject ) ) );

	if ( bSuccess )
		Log::Status( "SelfInstance", "Added object %s", a_pObject->GetRTTI().GetName().c_str() );
	else
		Log::Warning( "SelfInstance", "Failed to add object %s", a_pObject->GetRTTI().GetName().c_str() );

	return bSuccess;
}

bool SelfInstance::RemoveObject( ISerializable * a_pObject )
{
	if ( a_pObject == NULL )
		return false;

	std::string name( a_pObject->GetRTTI().GetName() );
	bool bSuccess = false;
	if ( a_pObject->GetRTTI().IsType( &IClassifier::GetStaticRTTI() ) )
	{
		IClassifier::SP spClassifier( static_cast<IClassifier *>( a_pObject )->shared_from_this() );
		if ( m_pClassifierManager->RemoveClassifier( spClassifier ) )
		{
			m_Classifiers.remove( spClassifier );
			bSuccess = true;
		}
	}
	else if ( a_pObject->GetRTTI().IsType( &IExtractor::GetStaticRTTI() ) )
	{
		IExtractor::SP spExtractor( static_cast<IExtractor *>( a_pObject )->shared_from_this() );
		if ( m_pFeatureManager->RemoveFeatureExtractor( spExtractor ) )
		{
			m_Extractors.remove( spExtractor );
			bSuccess = true;
		}
	}
	else if ( a_pObject->GetRTTI().IsType( &IAgent::GetStaticRTTI() ) )
	{
		IAgent::SP spAgent( static_cast<IAgent *>( a_pObject )->shared_from_this() );
		if ( m_pAgentSociety->RemoveAgent( spAgent ) )
		{
			m_Agents.remove( spAgent );
			bSuccess = true;
		}
	}
	else if ( a_pObject->GetRTTI().IsType( &ISensor::GetStaticRTTI() ) )
	{
		ISensor::SP spSensor( static_cast<ISensor *>( a_pObject )->shared_from_this() );
		if ( m_pSensorManager->RemoveSensor( spSensor ) )
		{
			m_Sensors.remove( spSensor );
			bSuccess = true;
		}
	}
	else if ( a_pObject->GetRTTI().IsType( &IService::GetStaticRTTI() ) )
		bSuccess = RemoveService( static_cast<IService *>( a_pObject )->shared_from_this() );
	else if ( a_pObject->GetRTTI().IsType( &ISkill::GetStaticRTTI() ) )
		bSuccess = m_pSkillManager->DeleteSkill( static_cast<ISkill *>( a_pObject )->shared_from_this() );
	else if ( a_pObject->GetRTTI().IsType( &IGesture::GetStaticRTTI() ) )
		bSuccess = m_pGestureManager->RemoveGesture( static_cast<IGesture *>( a_pObject )->shared_from_this() );
	else if ( a_pObject->GetRTTI().IsType( &Plan::GetStaticRTTI() ) )
		bSuccess = m_pPlanManager->DeletePlan( static_cast<Plan *>( a_pObject )->shared_from_this() );

	if ( bSuccess )
		Log::Status( "SelfInstance", "Removed object %s", name.c_str() );
	else
		Log::Warning( "SelfInstance", "Failed to remove object %s",name.c_str() );

	return bSuccess;
}


