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


#include "Status.h"

REG_SERIALIZABLE( Status );
RTTI_IMPL( Status, IThing );

void Status::Serialize(Json::Value & json)
{
	IThing::Serialize( json );

	json["m_State"] = m_State;
}

void Status::Deserialize(const Json::Value & json)
{
	IThing::Deserialize( json );

	if ( json.isMember("m_State") )
	{
		if(json["m_State"].asUInt() == 1)
			m_State = P_IDLE;
		else if(json["m_State"].asUInt() == 2)
			m_State = P_PROCESSING;
		else if(json["m_State"].asUInt() == 4)
			m_State = P_FINISHED;
	}
}