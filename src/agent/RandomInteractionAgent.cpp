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


#include "RandomInteractionAgent.h"
#include "blackboard/Text.h"
#include "blackboard/Goal.h"
#include "blackboard/Say.h"
#include "utils/TimerPool.h"

#include <stdlib.h>
#include <time.h>

REG_SERIALIZABLE(RandomInteractionAgent);
RTTI_IMPL(RandomInteractionAgent, IAgent);

void RandomInteractionAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
	json["m_fMinSpeakDelay"] = m_fMinSpeakDelay;
	json["m_fMaxSpeakDelay"] = m_fMaxSpeakDelay;
	json["m_RandomInteractionUtterance"] = m_RandomInteractionUtterance;
}

void RandomInteractionAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);

	if (json.isMember("m_fMinSpeakDelay"))
		m_fMinSpeakDelay = json["m_fMinSpeakDelay"].asFloat();
	if (json.isMember("m_fMaxSpeakDelay"))
		m_fMaxSpeakDelay = json["m_fMaxSpeakDelay"].asFloat();
	if (json.isMember("m_RandomInteractionUtterance"))
		m_RandomInteractionUtterance = json["m_RandomInteractionUtterance"].asString();
}

bool RandomInteractionAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->SubscribeToType("Say",
		DELEGATE(RandomInteractionAgent, OnInteraction, const ThingEvent &, this), TE_ADDED);
	pInstance->GetBlackBoard()->SubscribeToType("Text",
		DELEGATE(RandomInteractionAgent, OnInteraction, const ThingEvent &, this), TE_ADDED);

	StartTimer();
	return true;
}

bool RandomInteractionAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->UnsubscribeFromType("Say", this);
	pInstance->GetBlackBoard()->UnsubscribeFromType("Text", this);

	m_spSpeakTimer.reset();
	return true;
}

void RandomInteractionAgent::OnInteraction(const ThingEvent & a_ThingEvent)
{
	// start the timer over, this prevents us from saying anything while we have a goal..
	StartTimer();
}

void RandomInteractionAgent::StartTimer()
{
	TimerPool * pTimers = TimerPool::Instance();
	if (pTimers != NULL)
	{
		float fRandomFloat = ((float)(rand() % 1000)) / 1000.0f;
		float fDelay = m_fMaxSpeakDelay - ((m_fMaxSpeakDelay - m_fMinSpeakDelay) * fRandomFloat);
		m_spSpeakTimer = pTimers->StartTimer(VOID_DELEGATE(RandomInteractionAgent, OnSpeak, this),
			fDelay, true, false);

		Log::Debug("RandomInteractionAgent", "Speaking random phrase in %f seconds.", fDelay);
	}
}

void RandomInteractionAgent::OnSpeak()
{
	Text::SP spText(new Text(m_RandomInteractionUtterance, 1.0, false, true));
	SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spText);

	m_spSpeakTimer.reset();
	StartTimer();
}
