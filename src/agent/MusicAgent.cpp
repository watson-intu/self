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


#include "MusicAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "skills/SkillManager.h"

REG_SERIALIZABLE(MusicAgent);
RTTI_IMPL(MusicAgent, IAgent);

void MusicAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
	json["m_DanceSkillId"] = m_DanceSkillId;
}

void MusicAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
	if (json["m_DanceSkillId"].isString())
		m_DanceSkillId = json["m_DanceSkillId"].asString();
}

bool MusicAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert(pBlackboard != NULL);

	pBlackboard->SubscribeToType("MusicBeat",
		DELEGATE(MusicAgent, OnBeat, const ThingEvent &, this), TE_ADDED);
	pBlackboard->SubscribeToType("MusicStopped",
		DELEGATE(MusicAgent, OnBeat, const ThingEvent &, this), TE_ADDED);

	return true;
}

bool MusicAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert(pBlackboard != NULL);

	pBlackboard->UnsubscribeFromType("MusicBeat", this);
	pBlackboard->UnsubscribeFromType("MusicStopped", this);

	return true;
}

void MusicAgent::OnBeat(const ThingEvent & a_ThingEvent)
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance != NULL)
	{
		Log::Debug("MusicAgent", "Dancing using skill %s.", m_DanceSkillId.c_str());
		pInstance->GetSkillManager()->UseSkill(m_DanceSkillId, a_ThingEvent.GetIThing()->GetData(),
			a_ThingEvent.GetIThing());
	}
}

void MusicAgent::OnMusicStopped(const ThingEvent & a_ThingEvent)
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance != NULL)
		pInstance->GetSkillManager()->AbortActiveSkill(m_DanceSkillId);
}
