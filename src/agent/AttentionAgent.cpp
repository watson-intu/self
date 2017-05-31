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


#include "AttentionAgent.h"
#include "utils/TimerPool.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/IThing.h"
#include "blackboard/Say.h"
#include "blackboard/Gesture.h"
#include "blackboard/Attention.h"
#include "utils/Time.h"
#include "skills/ISkill.h"

REG_SERIALIZABLE(AttentionAgent);
RTTI_IMPL(AttentionAgent, IAgent);

void AttentionAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);

	json["m_WaitTime"] = m_WaitTime;
	json["m_ElevatedThresh"] = m_ElevatedThresh;
	json["m_StandardThresh"] = m_StandardThresh;
	json["m_LoweredThresh"] = m_LoweredThresh;
	json["m_MaxProximityDistance"] = m_MaxProximityDistance;
}

void AttentionAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);

	if (json.isMember("m_WaitTime"))
		m_WaitTime = json["m_WaitTime"].asFloat();
	if (json.isMember("m_ElevatedThresh"))
		m_ElevatedThresh = json["m_ElevatedThresh"].asFloat();
	if (json.isMember("m_StandardThresh"))
		m_StandardThresh = json["m_StandardThresh"].asFloat();
	if (json.isMember("m_LoweredThresh"))
		m_LoweredThresh = json["m_LoweredThresh"].asFloat();
	if (json.isMember("m_MaxProximityDistance"))
		m_MaxProximityDistance = json["m_MaxProximityDistance"].asFloat();
}

bool AttentionAgent::OnStart()
{
	SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType("Proximity",
		DELEGATE(AttentionAgent, OnProximity, const ThingEvent &, this), TE_ADDED);
	SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType("Gesture",
		DELEGATE(AttentionAgent, OnGesture, const ThingEvent &, this), TE_ADDED);

	m_spActionTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(AttentionAgent, OnAction, this), m_WaitTime, true, true);
	Log::Debug("AttentionAgent", "AttentionAgent started");
	return true;
}

bool AttentionAgent::OnStop()
{
	m_spWaitTimer.reset();
	m_spActionTimer.reset();
	SelfInstance::GetInstance()->GetBlackBoard()->UnsubscribeFromType("Proximity", this);
	SelfInstance::GetInstance()->GetBlackBoard()->UnsubscribeFromType("Gesture", this);

	Log::Debug("AttentionAgent", "AttentionAgent stopped");
	return true;
}

void AttentionAgent::OnGesture(const ThingEvent & a_ThingEvent)
{
	Gesture::SP spGesture = DynamicCast<Gesture>(a_ThingEvent.GetIThing());
	if (spGesture)
	{
		m_bGesture = true;
	}
}

void AttentionAgent::OnProximity(const ThingEvent & a_ThingEvent)
{
	Proximity::SP spProximity = DynamicCast<Proximity>(a_ThingEvent.GetIThing());
	if (spProximity)
	{
		if (spProximity->GetDistance() < m_MaxProximityDistance)
		{

			m_bProximity = true;
		}
	}
}

void AttentionAgent::OnCheck()
{
	Log::Debug("AttentionAgent", "Setting back to standard threshold");
	Attention::SP spAttention(new Attention(m_StandardThresh, m_StandardThresh - 0.2));
	SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spAttention);
}

void AttentionAgent::OnAction()
{
	if (m_bGesture || m_bProximity)
	{
		m_bProximity = false;
		m_bGesture = false;

		Log::Debug("AttentionAgent", "Attention found! Lowering threshold");
		Attention::SP spAttention(new Attention(m_LoweredThresh, m_LoweredThresh - 0.2));
		SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spAttention);

		m_spWaitTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(AttentionAgent, OnCheck, this), m_WaitTime + 1.0, true, false);
	}
}
