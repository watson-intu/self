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


#include "IAction.h"

RTTI_IMPL( IAction, ISerializable );

IAction::IAction() : 
	m_ActionId( UniqueID().Get() ),
	m_ActionFlags ( 0 ), 
	m_MinPreCondition( 0.5f ), 
	m_MinPostCondition( 0.5f )
{}

void IAction::Serialize(Json::Value & json)
{
	json["m_ActionId"] = m_ActionId;
	json["m_ActionFlags"] = m_ActionFlags;
	json["m_MinPreCondition"] = m_MinPreCondition;
	json["m_MinPostCondition"] = m_MinPostCondition;

	SerializeVector( "m_PreConditions", m_PreConditions, json );
	SerializeVector( "m_PostConditions", m_PostConditions, json );
}

void IAction::Deserialize(const Json::Value & json) 
{
	if ( json.isMember( "m_ActionId" ) )
		m_ActionId = json["m_ActionId"].asString();
	if ( json.isMember( "m_ActionFlags" ) )
		m_ActionFlags = json["m_ActionFlags"].asUInt();
	if ( json.isMember( "m_MinPreCondition" ) )
		m_MinPreCondition = json["m_MinPreCondition"].asFloat();
	if ( json.isMember( "m_MinPostCondition" ) )
		m_MinPostCondition = json["m_MinPostCondition"].asFloat();

	if ( json.isMember( "m_PreConditions" ) )
		DeserializeVector( "m_PreConditions", json, m_PreConditions );
	if ( json.isMember( "m_PostConditions" ) )
		DeserializeVector( "m_PostConditions", json, m_PostConditions );
}


