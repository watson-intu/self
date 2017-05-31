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


#ifndef SELF_INSTANCE_H
#define SELF_INSTANCE_H

#include <string>

#include "agent/IAgent.h"
#include "sensors/ISensor.h"
#include "classifiers/IClassifier.h"
#include "extractors/IExtractor.h"
#include "topics/ITopics.h"
#include "utils/Config.h"
#include "utils/GetMac.h"
#include "utils/IService.h"
#include "models/IGraph.h"

#include "SelfLib.h"			// include last

//! Forward declare
class BlackBoard;
class GestureManager;
class SkillManager;
class SensorManager;
class ClassifierManager;
class PlanManager;
class ThreadPool;
class FeatureManager;
class AgentSociety;
class TimerPool;
class TopicManager;

//! This class wraps most of the core systems, this is the main object to create to make a self instance.
class SELF_API SelfInstance : public Config
{
public:
	RTTI_DECL();

	//! Types
	typedef std::list<IAgent::SP>				AgentList;
	typedef std::list<ISensor::SP>				SensorList;
	typedef std::list<IExtractor::SP>	ExtractorList;
	typedef std::list<IClassifier::SP>			ClassifierList;
	typedef std::list<std::string>				FileList;
	typedef std::list<std::string>				PatternList;

	//! Data structure for holding instance specific data for self
	struct LocalConfig : public ISerializable
	{
		RTTI_DECL();

		LocalConfig() : m_RobotUrl( "tcp://127.0.0.1"), m_bUseDevVersion(false), m_nPort(-1)
		{}

		bool m_bUseDevVersion;

		std::string m_Name;				// name of this embodiment
		std::string m_RobotUrl;			// URL of the robot
		std::string m_MacId;			// mac Id
		std::string m_Ip;				// IP address
		std::string m_GroupId;			// Credentials
		std::string m_OrgId;
		std::string m_BearerToken;
		std::string m_EmbodimentId;		// Id of this embodiment
		std::string m_BodyId;			// Id of our body in the models of self
		std::string m_GatewayURL;		// URL for the gateway
		int m_nPort;					// port to listen on

		virtual void Serialize(Json::Value & json)
		{
			json["m_bUseDevVersion"] = m_bUseDevVersion;
			json["m_Name"] = m_Name;
			json["m_RobotUrl"] = m_RobotUrl;
			json["m_MacId"] = m_MacId;
			json["m_GroupId"] = m_GroupId;
			json["m_OrgId"] = m_OrgId;
			json["m_BearerToken"] = m_BearerToken;
			json["m_EmbodimentId"] = m_EmbodimentId;
			json["m_BodyId"] = m_BodyId;
			json["m_GatewayURL"] = m_GatewayURL;
			json["m_nPort"] = m_nPort;
		}

		virtual void Deserialize(const Json::Value & json)
		{
			if ( json["m_bUseDevVersion"].isBool())
				m_bUseDevVersion = json["m_bUseDevVersion"].asBool();
			if ( json["m_Name"].isString() )
				m_Name = json["m_Name"].asString();
			if ( json["m_RobotUrl"].isString() )
				m_RobotUrl = json["m_RobotUrl"].asString();
			if ( json["m_MacId"].isString() )
				m_MacId = json["m_MacId"].asString();
			if ( json["m_GroupId"].isString() )
				m_GroupId = json["m_GroupId"].asString();
			if ( json["m_OrgId"].isString() )
				m_OrgId = json["m_OrgId"].asString();
			if ( json["m_BearerToken"].isString() )
				m_BearerToken = json["m_BearerToken"].asString();
			if ( json["m_EmbodimentId"].isString() )
				m_EmbodimentId = json["m_EmbodimentId"].asString();
			if ( json["m_BodyId"].isString() )
				m_BodyId = json["m_BodyId"].asString();
			if ( json["m_GatewayURL"].isString() )
				m_GatewayURL = json["m_GatewayURL"].asString();
			if ( json["m_nPort"].isNumeric() )
				m_nPort = json["m_nPort"].asInt();
		}
	};

	//! Static instance
	static SelfInstance * GetInstance();

	//! Construction
	SelfInstance( const std::string & a_Platform,
		const std::string & a_staticDataPath = "./etc/",
		const std::string & a_InstanceDataPath = "./" );
	~SelfInstance();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Config interface
	virtual bool DisableLib( const std::string & a_Lib );

	//! Accessors
	bool					IsActive() const;
	const std::string &		GetPlatform() const;
	TopicManager *			GetTopicManager() const;
	ITopics *				GetTopics() const;
	BlackBoard *			GetBlackBoard() const;
	GestureManager *		GetGestureManager() const;
	SkillManager *			GetSkillManager() const;
	SensorManager *			GetSensorManager() const;
	ClassifierManager *		GetClassifierManager() const;
	PlanManager *			GetPlanManager() const;
	ThreadPool *			GetThreadPool() const;
	FeatureManager *		GetFeatureManager() const;
	AgentSociety *			GetAgentSociety() const;
	TimerPool *				GetTimerPool() const;

	const std::string &		GetInstanceId() const;			// a unique ID per instance of self, this changes each time a SelfInstance is created..
	LocalConfig &			GetLocalConfig();

	const IGraph::SP &		GetKnowledgeGraph() const;
	const std::string &		GetBackendLanguage() const;
	const std::string &		GetRobotType() const;
	const std::string &		GetLogoUrl() const;
	const AgentList &		GetAgentList() const;
	const SensorList &		GetSensorList() const;
	const ExtractorList &	GetExtractorList() const;
	const ClassifierList &	GetClassifierList() const;
	const std::string &		GetSelfVersion() const;
	const PatternList & 	GetInterfaces() const;
	const FileList &		GetGestureFiles() const;
	const FileList &		GetSkillFiles() const;
	const FileList &		GetPlanFiles() const;

	//! Initialize this self instance.
	bool					Start();
	//! Updates all systems of this self-instance, processes any pending events
	//! and queued data. Returns false if instance is stopped.
	bool					Update();
	//! Stop this self instance.
	bool					Stop();
	//! Main entry point for this SelfInstace, this calls Start(), Update(), then Stop()
	int						Run();

	//! Returns a agent object by it's type.
	template<typename T>
	T * FindAgent() const
	{
		for (AgentList::const_iterator iAgent = m_Agents.begin(); iAgent != m_Agents.end(); ++iAgent)
		{
			T * pAgent = DynamicCast<T>((*iAgent).get());
			if (pAgent != NULL)
				return pAgent;
		}

		return NULL;
	}

	template<typename T>
	T * FindClassifier() const
	{
		for (ClassifierList::const_iterator iClassifier = m_Classifiers.begin(); iClassifier != m_Classifiers.end(); ++iClassifier)
		{
			T * pClassifier = DynamicCast<T>((*iClassifier).get());
			if (pClassifier != NULL)
				return pClassifier;
		}

		return NULL;
	}

	void					SetBearerToken(const std::string & a_BearerToken);
	void					SetEmbodimentId(const std::string & a_EmbodimentId);

private:
	//! Types
	struct Event
	{
		Event( const ITopics::Payload & a_Payload );

		const ITopics::Payload & m_Payload;
		Json::Value	m_Event;
	};

	typedef std::map<std::string,Delegate<const Event &> >	EventHandlerMap;

	//! Data
	static SelfInstance *	sm_pInstance;
	bool					m_bActive;
	std::string				m_Platform;
	TopicManager *			m_pTopicManager;
	ITopics *				m_pTopics;
	BlackBoard *			m_pBlackBoard;
	GestureManager *		m_pGestureManager;
	SkillManager *			m_pSkillManager;
	SensorManager *			m_pSensorManager;
	ClassifierManager *		m_pClassifierManager;
	PlanManager *			m_pPlanManager;
	ThreadPool *			m_pThreadPool;
	FeatureManager *		m_pFeatureManager;
	AgentSociety *			m_pAgentSociety;
	TimerPool *				m_pTimerPool;

	std::string				m_KnowledgeGraphId;
	std::string				m_KnowledgeGraphType;
	std::string				m_KnowledgeGraphServiceId;
	IGraph::SP				m_spKnowledgeGraph;
	bool					m_bKnowledgeGraphReady;
	IVertex::SP				m_spMyBody;

	std::string				m_InstanceId;
	LocalConfig				m_LocalConfig;		// container for mutable data 

	std::string				m_BackendLanguage;	// Language that used by the back-end
	std::string				m_RobotType;		// type of robot
	std::string				m_LaunchUrl;		// URL to launch on startup
	std::string				m_LogoUrl;			// URL of logo image
	std::string				m_SelfVersion;		// Version of Self currently running
	float                   m_fDefaultVolume;
	AgentList				m_Agents;			// list of available agents to create
	SensorList				m_Sensors;			// list of sensors to create
	ExtractorList			m_Extractors;		// list of feature extractors
	ClassifierList			m_Classifiers;
	FileList				m_GestureFiles;		// list of files containing all gestures
	FileList				m_SkillFiles;
	FileList				m_PlanFiles;
	PatternList				m_Interfaces;
	TimerPool::ITimer::SP	m_spSaveInstanceData;
	bool					m_bSaveOnStop;

	EventHandlerMap			m_EventHandlers;

	void ReleaseObjects();
	void SendBacktraces();
	void ApplyLocalConfig();
	void OnSaveConfig();

	void OnKnowledgeGraphLoaded( IGraph::SP a_spGraph );

	void LoadBody();
	void SaveBody();
	void OnBodyLoaded(ITraverser::SP a_spTraverser);

	void RegisterEventHandlers();
	void OnConfigEvent( const ITopics::Payload & a_Payload );
	void OnLogLevel( const Event & a_Event );
	void OnGetConfig( const Event & a_Event );
	void OnListClasses( const Event & a_Event );
	void OnListLibraries( const Event & a_Event );
	void OnEnableLibrary( const Event & a_Event );
	void OnDisableLibrary( const Event & a_Event );
	void OnAddCredential( const Event & a_Event );
	void OnRemoveCredential( const Event & a_Event );
	void OnAddObject( const Event & a_Event );
	void OnRemoveObject( const Event & a_Event );
	void OnConfigTopicManager( const Event & a_Event );

	bool AddObject( ISerializable * a_pObject, bool a_bOverride );
	bool RemoveObject( ISerializable * a_pObject );
};

inline bool SelfInstance::IsActive() const
{
	return m_bActive;
}

inline const std::string & SelfInstance::GetPlatform() const
{
	return m_Platform;
}

inline TopicManager * SelfInstance::GetTopicManager() const
{
	return m_pTopicManager;
}

inline ITopics * SelfInstance::GetTopics() const
{
	return m_pTopics;
}

inline BlackBoard * SelfInstance::GetBlackBoard() const
{
	return m_pBlackBoard;
}

inline GestureManager * SelfInstance::GetGestureManager() const
{
	return m_pGestureManager;
}

inline SkillManager * SelfInstance::GetSkillManager() const
{
	return m_pSkillManager;
}

inline SensorManager * SelfInstance::GetSensorManager() const
{
	return m_pSensorManager;
}

inline ClassifierManager * SelfInstance::GetClassifierManager() const
{
	return m_pClassifierManager;
}

inline PlanManager * SelfInstance::GetPlanManager() const
{
	return m_pPlanManager;
}

inline ThreadPool * SelfInstance::GetThreadPool() const
{
	return m_pThreadPool;
}

inline FeatureManager * SelfInstance::GetFeatureManager() const
{
	return m_pFeatureManager;
}

inline AgentSociety * SelfInstance::GetAgentSociety() const
{
	return m_pAgentSociety;
}

inline TimerPool * SelfInstance::GetTimerPool() const
{
	return m_pTimerPool;
}

inline const IGraph::SP & SelfInstance::GetKnowledgeGraph() const
{
	return m_spKnowledgeGraph;
}

inline const std::string & SelfInstance::GetInstanceId() const
{
	return m_InstanceId;
}

inline SelfInstance::LocalConfig & SelfInstance::GetLocalConfig()
{
	return m_LocalConfig;
}

inline const std::string & SelfInstance::GetBackendLanguage() const
{
	return m_BackendLanguage;
}

inline const std::string & SelfInstance::GetRobotType() const
{
	return m_RobotType;
}

inline const std::string & SelfInstance::GetLogoUrl() const
{
	return m_LogoUrl;
}

inline const std::string & SelfInstance::GetSelfVersion() const
{
	return m_SelfVersion;
}

inline const SelfInstance::AgentList & SelfInstance::GetAgentList() const
{
	return m_Agents;
}

inline const SelfInstance::SensorList & SelfInstance::GetSensorList() const
{
	return m_Sensors;
}

inline const SelfInstance::ExtractorList & SelfInstance::GetExtractorList() const
{
	return m_Extractors;
}

inline const SelfInstance::ClassifierList &	SelfInstance::GetClassifierList() const
{
	return m_Classifiers;
}

inline const SelfInstance::PatternList & SelfInstance::GetInterfaces() const
{
	return m_Interfaces;
}

inline const SelfInstance::FileList & SelfInstance::GetGestureFiles() const
{
	return m_GestureFiles;
}

inline const SelfInstance::FileList & SelfInstance::GetSkillFiles() const
{
	return m_SkillFiles;
}

inline const SelfInstance::FileList & SelfInstance::GetPlanFiles() const
{
	return m_PlanFiles;
}

#endif
