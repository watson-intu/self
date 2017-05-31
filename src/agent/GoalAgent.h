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


#ifndef SELF_GOAL_AGENT_H
#define SELF_GOAL_AGENT_H

#include <list>
#include <algorithm>

#include "IAgent.h"
#include "planning/PlanInstance.h"
#include "blackboard/ThingEvent.h"
#include "SelfLib.h"

class Goal;

//! This goal manager watches for goals on the blackboard, and creates the plans
//! for those goals. It tracks those plans and updates the goals as the plan state changes.
class SELF_API GoalAgent : public IAgent
{
public:
	RTTI_DECL();

	//! Construction
    GoalAgent();
    ~GoalAgent();

	//! IAgent interface
	const char * GetName() const;
	bool OnStart();				
	bool OnStop();				

private:
	//! Callback for a new goal getting added to the blackboard
	void OnGoal(const ThingEvent &a_ThingEvent);
	//! Callback for a plan instance
	void OnPlanStateChanged(PlanInstance::SP a_spPlan);

	//! Types
	typedef std::set<PlanInstance::SP>	PlanSet;

	//! Data
	PlanSet			m_ActivePlans;
	Goal::SP		m_spPlanningGoal;
};

#endif //SELF_GOAL_AGENT_H
