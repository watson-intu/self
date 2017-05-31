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


#include "GestureAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Gesture.h"
#include "skills/SkillManager.h"


REG_SERIALIZABLE(GestureAgent);
RTTI_IMPL(GestureAgent, IAgent);

void GestureAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
}

void GestureAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
}

bool GestureAgent::OnStart()
{
	SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType(Gesture::GetStaticRTTI(),
		DELEGATE(GestureAgent, OnGesture, const ThingEvent &, this), TE_ADDED);

	return true;
}

bool GestureAgent::OnStop()
{
	SelfInstance::GetInstance()->GetBlackBoard()->UnsubscribeFromType(Gesture::GetStaticRTTI(), this);
	return true;
}

void GestureAgent::OnGesture(const ThingEvent & a_ThingEvent)
{
	Gesture::SP spGesture = DynamicCast<Gesture>(a_ThingEvent.GetIThing());
	if (spGesture)
	{
		Log::Debug("GestureAgent", "Using Skill: %s", spGesture->GetType().c_str());
		spGesture->SetState( "COMPLETED" );
		SelfInstance::GetInstance()->GetSkillManager()->UseSkill(spGesture->GetType(), spGesture->GetParams(),
			DELEGATE( GestureAgent, OnSkillState, SkillInstance *, this ), spGesture);
	}
}

void GestureAgent::OnSkillState(SkillInstance * a_pInstance)
{
	Gesture::SP spGesture = DynamicCast<Gesture>( a_pInstance->GetParent() );

	switch (a_pInstance->GetState())
	{
	case SkillInstance::US_COMPLETED:
	case SkillInstance::US_FAILED:
	case SkillInstance::US_UNAVAILABLE:
	case SkillInstance::US_ABORTED:
		spGesture->SetState( SkillInstance::StateToString( a_pInstance->GetState() ) );
		break;
	case SkillInstance::US_FINDING:
	case SkillInstance::US_EXECUTING:
		break;
	}

}
