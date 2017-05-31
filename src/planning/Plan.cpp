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


#include "Plan.h"
#include "utils/UniqueID.h"

REG_SERIALIZABLE(Plan);
RTTI_IMPL( Plan, ISerializable );

Plan::Plan() : m_bEnabled( true ), m_PlanId( UniqueID().Get() )
{}

void Plan::Serialize(Json::Value & json)
{
	json["m_bEnabled"] = m_bEnabled;
	json["m_PlanId" ] = m_PlanId;

	SerializeVector( "m_PreConditions", m_PreConditions, json );
	SerializeVector( "m_Actions", m_Actions, json );
	SerializeVector( "m_PostConditions", m_PostConditions, json );
}

void Plan::Deserialize(const Json::Value & json)
{
	if ( json["m_bEnabled"].isBool() )
		m_bEnabled = json["m_bEnabled"].asBool();
	if ( json["m_PlanId"].isString() )
		m_PlanId = json["m_PlanId"].asString();

	DeserializeVector("m_PreConditions", json, m_PreConditions);
	DeserializeVector("m_Actions", json, m_Actions);
	DeserializeVector( "m_PostConditions", json, m_PostConditions );
}


//! Check if this plan is a valid plan for the given goal object, returns a value between 0.0 (no) to 1.0 (yes)
float Plan::TestPreConditions( Goal::SP a_spGoal )
{
	float fValid = 1.0f;
	for( size_t i=0;i<m_PreConditions.size();++i)
		fValid *= m_PreConditions[i]->Test( a_spGoal );

	return fValid;
}

float Plan::TestPostConditions( Goal::SP a_spGoal )
{
	float fValid = 1.0f;
	for( size_t i=0;i<m_PostConditions.size();++i)
		fValid *= m_PostConditions[i]->Test( a_spGoal );

	return fValid;
}


