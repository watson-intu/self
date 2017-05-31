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


#include "GoalParamsConditon.h"

REG_SERIALIZABLE(GoalParamsCondition);
RTTI_IMPL(GoalParamsCondition, ICondition);

void GoalParamsCondition::Serialize(Json::Value & json)
{
	ICondition::Serialize(json);

	for (size_t i = 0; i < m_Params.size(); ++i)
	{
		json["m_Params"][i]["m_Name"] = m_Params[i].m_Name;
		json["m_Params"][i]["m_Value"] = m_Params[i].m_Value;
		json["m_Params"][i]["m_Op"] = EqualityOpText(m_Params[i].m_Op);
	}
	json["m_LogicalOp"] = LogicalOpText(m_LogicalOp);
}

void GoalParamsCondition::Deserialize(const Json::Value & json)
{
	ICondition::Deserialize(json);

	m_Params.clear();

	const Json::Value & elements = json["m_Params"];
	for (Json::ValueConstIterator iElement = elements.begin(); iElement != elements.end(); ++iElement)
	{
		m_Params.push_back( ParamCondition() );

		ParamCondition & c = m_Params.back();
		c.m_Name = (*iElement)["m_Name"].asString();
		c.m_Value = (*iElement)["m_Value"];
		c.m_Op = GetEqualityOp(( *iElement)["m_Op"].asString() );
	}
	m_LogicalOp = GetLogicalOp(json["m_LogicalOp"].asString());
}

//! ICondition interface
float GoalParamsCondition::Test(Goal::SP a_spGoal)
{
	const ParamsMap & params = a_spGoal->GetParams();

	std::vector<bool> values;
	for (size_t i = 0; i < m_Params.size(); ++i)
	{
		Json::Value value;
		if (params.ValidPath(m_Params[i].m_Name))
			value = params[m_Params[i].m_Name];

		values.push_back( TestEqualityOp(m_Params[i].m_Op, m_Params[i].m_Value, value) );
	}

	if (TestLogicalOp(m_LogicalOp, values))
		return 1.0f;

	return 0.0f;
}

ICondition * GoalParamsCondition::Clone()
{
	return new GoalParamsCondition(*this);
}

