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


#include "Health.h"

REG_SERIALIZABLE( Health );
RTTI_IMPL( Health, IThing );

void Health::Serialize(Json::Value & json)
{
    IThing::Serialize( json );
	json["m_HealthFamily"] = m_HealthFamily;
    json["m_HealthName"] = m_HealthName;
	json["m_fHealthValue"] = m_fHealthValue;
}

void Health::Deserialize(const Json::Value & json)
{
    IThing::Deserialize( json );

	if (json.isMember("m_HealthFamily"))
		m_HealthFamily = json["m_HealthFamily"].asString();
    if ( json.isMember("m_HealthName") )
        m_HealthName = json["m_HealthName"].asString();
	if ( json.isMember("m_fHealthValue") )
		m_fHealthValue = json["m_fHealthValue"].asFloat();
}

