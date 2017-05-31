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


#include "AsimovAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "skills/SkillManager.h"

REG_SERIALIZABLE(AsimovAgent);
RTTI_IMPL(AsimovAgent, IAgent);

void AsimovAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
	SerializeVector("m_Interruptions", m_Interruptions, json);
}

void AsimovAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
	DeserializeVector("m_Interruptions", json, m_Interruptions);

	if (m_Interruptions.size() == 0)
		m_Interruptions.push_back("stop");
}

bool AsimovAgent::OnStart()
{
	Log::Debug("AsimovAgent", "Starting AsimovAgent");
	SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType("Text",
		DELEGATE(AsimovAgent, OnText, const ThingEvent &, this), TE_ADDED);
	return true;
}

bool AsimovAgent::OnStop()
{
	Log::Debug("AsimovAgent", "Stopping AsimovAgent");
	SelfInstance::GetInstance()->GetBlackBoard()->UnsubscribeFromType( "Text", this);
	return true;
}

void AsimovAgent::OnText(const ThingEvent & a_ThingEvent)
{
	Text::SP spText = DynamicCast<Text>(a_ThingEvent.GetIThing());
	if (spText)
	{
		for (size_t i = 0; i < m_Interruptions.size(); ++i)
		{
			if (spText->GetText() == m_Interruptions[i])
			{
				Log::Debug("AsimovAgent", "Aborting all skills!!!");
				SelfInstance::GetInstance()->GetSkillManager()->AbortActiveSkills();
				break;
			}
		}
	}
}