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


#ifndef SELF_GOAL_PARAMS_CONDTION_H
#define SELF_GOAL_PARAMS_CONDTION_H

#include "planning/ICondition.h"

//! This condition tests against one or more of the params of the Goal object.
struct SELF_API GoalParamsCondition : public ICondition
{
	RTTI_DECL();

	//! Types
	struct ParamCondition
	{
		ParamCondition() : m_Op(EQ)
		{}
		ParamCondition(const std::string & a_Name, const Json::Value & a_Value, EqualityOp a_Op) :
			m_Name(a_Name),
			m_Op(a_Op),
			m_Value(a_Value)
		{}

		std::string		m_Name;
		Json::Value		m_Value;
		EqualityOp		m_Op;
	};

	//! COnstructions
	GoalParamsCondition() : m_LogicalOp(AND)
	{}
	GoalParamsCondition(const std::vector<ParamCondition> & a_Params, LogicalOp a_LogOp = AND) :
		m_Params(a_Params), m_LogicalOp(a_LogOp)
	{}
	GoalParamsCondition(const std::string & a_Name, const  Json::Value & a_Value, 
		EqualityOp a_Op = ICondition::EQ, LogicalOp a_LogOp = AND) : m_LogicalOp(a_LogOp)
	{
		m_Params.push_back(ParamCondition(a_Name, a_Value, a_Op));
	}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! ICondition interface
	virtual float Test(Goal::SP a_spGoal);
	virtual ICondition * Clone();

	std::vector<ParamCondition>
						m_Params;
	LogicalOp			m_LogicalOp;
};

#endif
