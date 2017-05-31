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


#ifndef SELF_GOAL_TYPE_CONDTION_H
#define SELF_GOAL_TYPE_CONDTION_H

#include "planning/ICondition.h"

//! This condition tests against the type of goal
struct SELF_API GoalTypeCondition : public ICondition
{
	RTTI_DECL();

	GoalTypeCondition() : m_GoalTypeOp(EQ)
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! ICondition interface
	virtual float Test(Goal::SP a_spGoal);
	virtual ICondition * Clone();

	std::string			m_GoalType;
	EqualityOp			m_GoalTypeOp;
};

#endif
