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


#ifndef SELF_REQUESTAGENT_H
#define SELF_REQUESTAGENT_H

#include <list>

#include "IAgent.h"
#include "utils/Factory.h"
#include "blackboard/Entity.h"
#include "blackboard/Image.h"
#include "blackboard/RequestIntent.h"

#include "SelfLib.h"

//! Forward Declarations
class SelfInstance;

class SELF_API RequestAgent : public IAgent
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr< RequestAgent >		SP;
	typedef boost::weak_ptr< RequestAgent >			WP;

	//! Construction
	RequestAgent();

	//! ISerializable interface
	void Serialize( Json::Value & json );
	void Deserialize( const Json::Value & json );

    //! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Types
	struct SaveAction
	{
		SaveAction( RequestAgent * a_pAgent, const Json::Value & a_Props );
		void OnTraverse( ITraverser::SP a_spTraverser );

		RequestAgent *			m_pAgent;
		Json::Value				m_Props;
	};
	struct FindAction
	{
		FindAction( const SP & a_spAgent,
			const RequestIntent::SP & a_spIntent, 
			const std::string & a_Verb, 
			const std::string & a_Noun );
		void OnTraverseRequest( ITraverser::SP a_spTraverser );
		void CreateGoal();

		SP						m_spAgent;
		RequestIntent::SP		m_spIntent;
		std::string				m_Verb;
		std::string				m_Noun;
		std::string				m_Skill;
	};

	//! Event Handlers
    void OnRequestIntent(const ThingEvent & a_ThingEvent);
	void OnRecognizedObject(const ThingEvent & a_ThingEvent);
	void OnGoalState( const ThingEvent & a_Event );

	//! Types
	typedef std::list< Entity::SP >				EntityList;
	typedef std::map<std::string,IThing::SP>	ObjectMap;

	//! Data
	std::vector<std::string>	m_RequestFiles;
	std::vector<std::string>	m_RequestFailedText;
	std::vector<std::string>	m_EntityFilter;
	ObjectMap					m_RecognizedObjects;
	volatile int				m_nPendingOps;
};

#endif //SELF_REQUESTAGENT_H