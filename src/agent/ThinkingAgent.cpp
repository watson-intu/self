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


#include "ThinkingAgent.h"
#include "blackboard/QuestionIntent.h"
#include "blackboard/Say.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Status.h"
#include "skills/SkillManager.h"
#include "skills/SkillInstance.h"
#include "SelfInstance.h"

REG_SERIALIZABLE(ThinkingAgent);
RTTI_IMPL(ThinkingAgent, IAgent);

void ThinkingAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);

	json["m_fProcessingTime"] = m_fProcessingTime;
	for (size_t i = 0; i < m_PleaseWaitText.size(); ++i)
		json["m_PleaseWaitText"].append(m_PleaseWaitText[i]);
}

void ThinkingAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);

	if (json.isMember("m_fProcessingTime"))
		m_fProcessingTime = json["m_fProcessingTime"].asFloat();

	if (json.isMember("m_PleaseWaitText"))
	{
		m_PleaseWaitText.clear();

		const Json::Value & text = json["m_PleaseWaitText"];
		for (Json::ValueConstIterator iText = text.begin(); iText != text.end(); ++iText)
			m_PleaseWaitText.push_back((*iText).asString());
	}

	// add a default line if none is serialized in..
	if (m_PleaseWaitText.size() == 0)
		m_PleaseWaitText.push_back("ummmm...");
}

bool ThinkingAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;

	pInstance->GetBlackBoard()->SubscribeToType( "IIntent",
		DELEGATE(ThinkingAgent, OnIntent, const ThingEvent &, this), TE_STATE);
	pInstance->GetBlackBoard()->SubscribeToType( "Status",
		DELEGATE(ThinkingAgent, OnStatus, const ThingEvent &, this), TE_ADDED_OR_STATE);

	return true;
}

bool ThinkingAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;

	pInstance->GetBlackBoard()->UnsubscribeFromType( "IIntent", this);
	pInstance->GetBlackBoard()->UnsubscribeFromType( "Status", this);
	m_ProcessingThingList.clear();

	return true;
}

void ThinkingAgent::OnIntent(const ThingEvent & a_ThingEvent)
{
	if (a_ThingEvent.GetIThing()->GetState() == "PROCESSING")
	{
		//New IThing changed its state to processing
		//We need to add it to our data structure and kick off a timer thread for it
		m_ProcessingThingList.push_back(ProcessingIThingSP( new ProcessingIThing(this, a_ThingEvent.GetIThing())) );
	}
	else
	{
		//Check to see if we have the goal already in our list to see if the state is no longer processing (and we can cancel the timer)
		for (ProcessingThingList::iterator iterator = m_ProcessingThingList.begin(),
			end = m_ProcessingThingList.end(); iterator != end; ++iterator)
		{
			if (a_ThingEvent.GetIThing()->GetGUID() == (*iterator)->m_spThing->GetGUID())
			{
				//We're already monitoring this goal, let's kill its timer
				m_ProcessingThingList.erase(iterator);
				break;
			}
		}
	}
}

void ThinkingAgent::OnStatus(const ThingEvent & a_ThingEvent)
{
	Status::SP spStatus = DynamicCast<Status>(a_ThingEvent.GetIThing());
	SkillManager * pSkillMgr = SelfInstance::GetInstance()->GetSkillManager();
	if (spStatus->GetState() == Status::P_IDLE)
		pSkillMgr->UseSkill("update_status", ParamsMap("status", "IDLE"), spStatus);
	else if (spStatus->GetState() == Status::P_PROCESSING)
		pSkillMgr->UseSkill("update_status", ParamsMap("status", "PROCESSING"), spStatus);
	else if (spStatus->GetState() == Status::P_FINISHED)
		pSkillMgr->UseSkill("update_status", ParamsMap("status", "FINISHED"), spStatus);
}

ThinkingAgent::ProcessingIThing::ProcessingIThing(ThinkingAgent * a_pAgent, IThing::SP a_spThing) :
	m_pAgent(a_pAgent), m_spThing(a_spThing), m_spStatus( new Status() )
{
	m_spStatus->SetState( Status::P_PROCESSING );
	m_spThing->AddChild( m_spStatus );

	m_spTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(ProcessingIThing, OnTimer, this), m_pAgent->m_fProcessingTime, true, false);
}

ThinkingAgent::ProcessingIThing::ProcessingIThing(const ProcessingIThing & a_Copy) :
	m_pAgent(a_Copy.m_pAgent), m_spThing(a_Copy.m_spThing), m_spStatus( new Status() )
{
	m_spStatus->SetState( Status::P_PROCESSING );
	m_spThing->AddChild( m_spStatus );

	m_spTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(ProcessingIThing, OnTimer, this), m_pAgent->m_fProcessingTime, true, false);
}

ThinkingAgent::ProcessingIThing::~ProcessingIThing()
{
	m_spStatus->SetState( Status::P_FINISHED );
}

void ThinkingAgent::ProcessingIThing::OnTimer()
{
	const std::vector<std::string> & text = m_pAgent->m_PleaseWaitText;
	if (text.size() > 0)
		m_spThing->AddChild(Say::SP(new Say(text[rand() % text.size()])));
	m_spTimer.reset();
}

