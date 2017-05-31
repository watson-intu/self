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


#include "DisplayAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Goal.h"
#include "skills/SkillManager.h"

REG_SERIALIZABLE(DisplayAgent);
RTTI_IMPL(DisplayAgent, IAgent);

DisplayAgent::DisplayAgent() :
	m_DisplaySkill("display")
{}

void DisplayAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
	json["m_DisplaySkill"] = m_DisplaySkill;
}

void DisplayAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
	if (json.isMember("m_DisplaySkill"))
		m_DisplaySkill = json["m_DisplaySkill"].asString();
}

bool DisplayAgent::OnStart()
{
	Log::Debug("DisplayAgent", "Display Agent has started!");
	SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType("Display",
		DELEGATE(DisplayAgent, OnDisplay, const ThingEvent &, this), TE_ADDED);

	return true;
}

bool DisplayAgent::OnStop()
{
	Log::Debug("DisplayAgent", "Display Agent has stopped!");
	SelfInstance::GetInstance()->GetBlackBoard()->UnsubscribeFromType("Display", this);
	return true;
}

void DisplayAgent::OnDisplay(const ThingEvent & a_ThingEvent)
{
	Display::SP spDisplay = DynamicCast<Display>(a_ThingEvent.GetIThing());
	if (spDisplay)
	{
		if (!m_spActive)
		{
			ExecuteDisplay(spDisplay);
		}
		else
		{
			// dialog already active, just push into the queue
			m_Displays.push_back(spDisplay);
		}
	}
}

void DisplayAgent::ExecuteDisplay(Display::SP a_pDisplay)
{
	Log::Debug("DisplayAgent", "Display object added to blackboard detected");

	SelfInstance * pInstance = SelfInstance::GetInstance();
	m_spActive = a_pDisplay;
	m_spActive->SetState("PROCESSING");

	ParamsMap params;
	params["display"] = a_pDisplay->GetDisplay();
	params["data"] = a_pDisplay->GetData();

	boost::shared_ptr<DisplayAgent> spThis( boost::static_pointer_cast<DisplayAgent>( shared_from_this() ) );
	pInstance->GetSkillManager()->UseSkill(m_DisplaySkill, params,
		DELEGATE(DisplayAgent, OnSkillState, SkillInstance *, spThis), m_spActive);
}

void DisplayAgent::OnSkillState(SkillInstance * a_pInstance)
{
	if (a_pInstance == NULL) {
		Log::Error("DisplayAgent", "Received NULL skill state after executing dialog");
		return;
	}

	SkillInstance::UseSkillState state = a_pInstance->GetState();
	const std::string &stateName = a_pInstance->StateToString(state);

	// Update the state
	m_spActive->SetState(stateName);

	switch (state) {
		// the phrase has been either completed or aborted - continue to next phrase if available
	case SkillInstance::US_COMPLETED:
	case SkillInstance::US_ABORTED: {
		if (m_Displays.begin() != m_Displays.end()) {
			Display::SP spDisplay = m_Displays.front();
			m_Displays.pop_front();

			ExecuteDisplay(spDisplay);
		}
		else
			m_spActive.reset();
	}
									break;

									// Something went wrong with the skill - report the failure
	case SkillInstance::US_FAILED:
	case SkillInstance::US_UNAVAILABLE:
		m_Displays.clear();
		m_spActive.reset();
		break;

		// the skill is current is working, let him continue
	case SkillInstance::US_FINDING:
	case SkillInstance::US_EXECUTING:
		// No op. We logged that already, there is nothing else to do
		break;

	case SkillInstance::US_INVALID:
		m_spActive.reset();
		break;
	}
}