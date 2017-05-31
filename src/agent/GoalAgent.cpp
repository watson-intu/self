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


#include "GoalAgent.h"
#include "SelfInstance.h"

#include "planning/PlanManager.h"
#include "blackboard/Goal.h"

REG_SERIALIZABLE(GoalAgent);
RTTI_IMPL(GoalAgent, IAgent);

GoalAgent::GoalAgent() 
{}

GoalAgent::~GoalAgent()
{}

bool GoalAgent::OnStart()
{
	// TODO: need to change the logic so a goal is held in queue and not started if attached
	// to an incomplete goal.
	SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType( "Goal",
		DELEGATE(GoalAgent, OnGoal, const ThingEvent &, this), TE_ADDED_OR_STATE );
	return true;
}

bool GoalAgent::OnStop()
{
	SelfInstance::GetInstance()->GetBlackBoard()->UnsubscribeFromType( "Goal", this);
    return true;
}

void GoalAgent::OnGoal(const ThingEvent & a_ThingEvent)
{
	Goal::SP spGoal = DynamicCast<Goal>(a_ThingEvent.GetIThing());
	if ( spGoal->GetState() == "ADDED" || spGoal->GetState() == "FAILED" )
	{
		Log::Debug("GoalAgent", "OnGoalState(), Goal: %s, Type: %s, State: %s", spGoal->GetGUID().c_str(),
			spGoal->GetRTTI().GetName().c_str(), spGoal->GetState().c_str());

		// find and execute a plan for this goal.
		boost::shared_ptr<GoalAgent> spThis( boost::static_pointer_cast<GoalAgent>( shared_from_this() ) );
		PlanInstance::SP spInstance = SelfInstance::GetInstance()->GetPlanManager()->ExecutePlan(spGoal,
			DELEGATE( GoalAgent, OnPlanStateChanged, PlanInstance::SP, spThis) );
		if ( spInstance && (spInstance->GetState() == PlanInstance::FINDING 
			|| spInstance->GetState() == PlanInstance::EXECUTING) )
		{
			m_ActivePlans.insert( spInstance );
		}
	}
}

void GoalAgent::OnPlanStateChanged(PlanInstance::SP a_spPlan)
{
	// update the goal state based on the plan state..
	Goal::SP spGoal = a_spPlan->GetGoal();
	if (spGoal)
	{
		if (a_spPlan->GetState() == PlanInstance::FINDING )
		{
			Log::Debug("GoalManager", "Goal %s finding plan...", spGoal->GetGUID().c_str());
		}
		else if (a_spPlan->GetState() == PlanInstance::EXECUTING)
		{
			Log::Debug("GoalManager", "Goal %s processing...", spGoal->GetGUID().c_str());
			spGoal->SetState("PROCESSING");
		}
		else if (a_spPlan->GetState() == PlanInstance::COMPLETED)
		{
			Log::Debug("GoalManager", "... Goal %s completed.", spGoal->GetGUID().c_str());
			spGoal->SetState("COMPLETED");

			m_ActivePlans.erase(a_spPlan);
		}
		else 
		{
			// do not fail the goal if it's already COMPLETED or FAILED.
			Log::Debug("GoalManager", "... Goal %s FAILED! (Plan State: %s)", 
				spGoal->GetGUID().c_str(), PlanInstance::GetPlanStateText( a_spPlan->GetState() ) );
			spGoal->SetState("FAILED");

			m_ActivePlans.erase(a_spPlan);
		}
	}

}

