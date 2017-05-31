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


#include "NameAgent.h"
#include "SelfInstance.h"
#include "blackboard/NameIntent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Image.h"
#include "blackboard/Say.h"
#include "blackboard/Text.h"
#include "classifiers/FaceClassifier.h"
#include "services/IGateway.h"

REG_SERIALIZABLE(NameAgent);
RTTI_IMPL(NameAgent, IAgent);

NameAgent::NameAgent() :
	m_MeEntity("me"),
	m_YouEntity("you")
{}

void NameAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);

	SerializeVector("m_LearnNewPerson", m_LearnNewPerson, json);
	SerializeVector("m_LearnNewName", m_LearnNewName, json);
	SerializeVector("m_LearnNewNameFail", m_LearnNewNameFail, json);
	SerializeVector("m_LearnNewBlindPerson", m_LearnNewBlindPerson, json);

	json["m_MeEntity"] = m_MeEntity;
	json["m_YouEntity"] = m_YouEntity;
}

void NameAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);

	DeserializeVector("m_LearnNewPerson", json, m_LearnNewPerson);
	DeserializeVector("m_LearnNewName", json, m_LearnNewName);
	DeserializeVector("m_LearnNewNameFail", json, m_LearnNewNameFail);
	DeserializeVector("m_LearnNewBlindPerson", json, m_LearnNewBlindPerson);

	m_MeEntity = json["m_MeEntity"].asString();
	m_YouEntity = json["m_YouEntity"].asString();

	if (m_LearnNewPerson.size() == 0)
		m_LearnNewPerson.push_back("It is a pleasure to meet you %s.");
	if (m_LearnNewName.size() == 0)
		m_LearnNewName.push_back("Ok, I will go by %s now");
	if (m_LearnNewNameFail.size() == 0)
		m_LearnNewNameFail.push_back("Sorry but I cannot rename myself that");
	if (m_LearnNewBlindPerson.size() == 0)
		m_LearnNewBlindPerson.push_back("Hi %s, I'm having a little trouble seeing you. Please step in front of the camera.");
}

bool NameAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert( pBlackboard != NULL );

	pBlackboard->SubscribeToType("NameIntent", DELEGATE(NameAgent, OnNameIntent, const ThingEvent &, this), TE_ADDED);
	pBlackboard->SubscribeToType("Person", DELEGATE(NameAgent, OnPerson, const ThingEvent &, this), TE_ALL);
	return true;
}

bool NameAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert( pBlackboard != NULL );

	pBlackboard->UnsubscribeFromType("NameIntent", this);
	pBlackboard->UnsubscribeFromType("Person", this);
	return true;
}

void NameAgent::OnPerson(const ThingEvent & a_ThingEvent)
{
	Person::SP spPerson = DynamicCast<Person>(a_ThingEvent.GetIThing());
	if (spPerson)
	{
		if (a_ThingEvent.GetThingEventType() == TE_ADDED)
			m_ActivePersons.push_front(spPerson);
		else if (a_ThingEvent.GetThingEventType() == TE_REMOVED)
			m_ActivePersons.remove(spPerson);
	}
}

void NameAgent::OnNameIntent(const ThingEvent & a_ThingEvent)
{
	NameIntent::SP spNameIntent = DynamicCast<NameIntent>(a_ThingEvent.GetIThing());
	if (spNameIntent)
	{
		if (spNameIntent->GetPossessive() == m_MeEntity)
			OnLearnNewPerson(spNameIntent);
		else
			OnLearnNewName(spNameIntent);
	}
}

void NameAgent::OnLearnNewName(NameIntent::SP spNameIntent)
{
	std::string name = spNameIntent->GetName();
	if (!name.empty())
	{
		SelfInstance::GetInstance()->GetLocalConfig().m_Name = name;

		IGateway *pGateway = SelfInstance::GetInstance()->FindService<IGateway>();
		if (pGateway != NULL && pGateway->IsConfigured())
			pGateway->UpdateEmbodimentName(DELEGATE(NameAgent, OnRegisterEmbodiment, const Json::Value &, this));

		std::string response(StringUtil::Format(m_LearnNewName[rand() % m_LearnNewName.size()].c_str(), name.c_str()));
		spNameIntent->AddChild(Say::SP(new Say(response)));
	}
	else
	{
		std::string response(StringUtil::Format(m_LearnNewNameFail[rand() % m_LearnNewNameFail.size()].c_str()));
		spNameIntent->AddChild(Say::SP(new Say(response)));
	}
	spNameIntent->SetState("COMPLETED");
}

void NameAgent::OnRegisterEmbodiment(const Json::Value & a_Response)
{
	if (!a_Response.isNull())
	{
		Log::Debug("NameAgent", "OnRegisterEmbodiment: %s", a_Response.toStyledString().c_str());
		if (a_Response["embodimentName"].isString())
		{
			if (a_Response["embodimentName"].asString().compare(SelfInstance::GetInstance()->GetLocalConfig().m_Name) != 0)
				Log::Error("NameAgent", "Failed to update name to the Intu Gateway");
		}
	}
}

void NameAgent::OnLearnNewPerson(NameIntent::SP spNameIntent)
{
	std::string name = spNameIntent->GetName();
	if (!name.empty())
	{
		FaceClassifier * pClassifier = SelfInstance::GetInstance()->FindClassifier<FaceClassifier>();
		if (pClassifier != NULL)
		{
			Person::SP spNamePerson, spRenamePerson;

			// TODO: it would be nice to have directional sound info in the NameIntent, so we know which person might be talking. Lets
			// assume when that is working, we will attach the NameIntent as a child of the person object to make it easy.
			// search all active person, use the person that doesn't have a name yet.
			for (PersonList::iterator iPerson = m_ActivePersons.begin();
				iPerson != m_ActivePersons.end(); ++iPerson)
			{
				Person::SP spPerson = *iPerson;
				if (!spPerson->FindChildType("Features"))
					continue;		// no features, just ignore..

				if (!spPerson->FindChildType("RecognizedFace"))
				{
					spNamePerson = spPerson;
					break;
				}

				if (!spRenamePerson)
					spRenamePerson = spPerson;
			}

			if (!spNamePerson)
				spNamePerson = spRenamePerson;

			if (spNamePerson)
			{
				if (pClassifier->LearnPerson(spNamePerson, name))
				{
					std::string response(StringUtil::Format(m_LearnNewPerson[rand() % m_LearnNewPerson.size()].c_str(),
						name.c_str()));
					spNameIntent->AddChild(Say::SP(new Say(response)));
				}
				else
				{
					Log::Error("NameAgent", "Failed to learn face");
				}
			}
			else
			{
				Log::Error("NameAgent", "No valid person found to set name.");
				std::string response(StringUtil::Format(m_LearnNewBlindPerson[rand() % m_LearnNewBlindPerson.size()].c_str(),
					name.c_str()));
				spNameIntent->AddChild(Say::SP(new Say(response)));
			}
		}
		else
		{
			std::string response(StringUtil::Format(m_LearnNewPerson[rand() % m_LearnNewPerson.size()].c_str(),
				name.c_str()));
			spNameIntent->AddChild(Say::SP(new Say(response)));
		}
	}
}

