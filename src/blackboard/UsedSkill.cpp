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


#include "UsedSkill.h"

RTTI_IMPL( UsedSkill, IThing );
REG_SERIALIZABLE( UsedSkill );

//! ISerializable interface
void UsedSkill::Serialize( Json::Value & json )
{
	IThing::Serialize( json );

	json["m_spSkill"] = ISerializable::SerializeObject( m_spSkill.get() );
	json["m_bFailed"] = m_bFailed;
}

void UsedSkill::Deserialize( const Json::Value & json )
{
	IThing::Deserialize( json );

	m_spSkill = ISkill::SP( ISerializable::DeserializeObject<ISkill>( json["m_spSkill"] ) );
	m_bFailed = json["m_bFailed"].asBool();
}

