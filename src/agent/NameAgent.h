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


#ifndef SELF_NAMEAGENT_H
#define SELF_NAMEAGENT_H

#include "IAgent.h"
#include "blackboard/NameIntent.h"
#include "blackboard/Person.h"
#include "blackboard/Image.h"
#include "blackboard/Entity.h"

#include "SelfLib.h"

class SELF_API NameAgent : public IAgent
{
public:
	RTTI_DECL();

	NameAgent();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Callbacks
	void OnPerson(const ThingEvent & a_ThingEvent);
	void OnNameIntent(const ThingEvent & a_ThingEvent);
	void OnLearnNewName(NameIntent::SP spNameIntent);
	void OnLearnNewPerson(NameIntent::SP spNameIntent);
	void OnRegisterEmbodiment(const Json::Value & a_Response);

	//! Types
	typedef std::list<Person::SP>	PersonList;

	//! Data
	std::vector<std::string>		m_LearnNewPerson;
	std::vector<std::string>		m_LearnNewName;
	std::vector<std::string>		m_LearnNewNameFail;
	std::vector<std::string>		m_LearnNewBlindPerson;
	PersonList						m_ActivePersons;
	std::string						m_MeEntity;
	std::string						m_YouEntity;

};

#endif // SELF_NAMEAGENT_H