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


#include "ConditionList.h"

REG_SERIALIZABLE(ConditionList);
RTTI_IMPL(ConditionList, ICondition);

void ConditionList::Serialize(Json::Value & json)
{
	SerializeVector("m_Conditions", m_Conditions, json );

	json["m_LogicalOp"] = LogicalOpText(m_LogicalOp);
	json["m_NOT"] = m_NOT;
	json["m_fMinTrue"] = m_fMinTrue;
}

void ConditionList::Deserialize(const Json::Value & json)
{
	DeserializeVector("m_Conditions", json, m_Conditions);

	m_LogicalOp = GetLogicalOp(json["m_LogicalOp"].asString());
	m_NOT = json["m_NOT"].asBool();
	m_fMinTrue = json["m_fMinTrue"].asFloat();
}

float ConditionList::Test(Goal::SP a_spGoal)
{
	std::vector<bool> values;
	for (size_t i = 0; i < m_Conditions.size(); ++i)
		values.push_back(m_Conditions[i]->Test(a_spGoal) >= m_fMinTrue);

	bool bResult = TestLogicalOp(m_LogicalOp, values);
	if (m_NOT)
		bResult = !bResult;

	return bResult ? 1.0f : 0.0f;
}

ICondition * ConditionList::Clone()
{
	return new ConditionList(*this);
}

