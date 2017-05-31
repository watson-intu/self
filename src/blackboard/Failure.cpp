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


#include "Failure.h"

RTTI_IMPL( Failure, IThing );
REG_SERIALIZABLE( Failure );

void Failure::Serialize(Json::Value & json)
{
	IThing::Serialize( json );

	json["m_Name"] = m_Name;
	json["m_Info"] = m_Info;
	json["m_Confidence"] = m_Confidence;
	json["m_Threshold"] = m_Threshold;
}
void Failure::Deserialize(const Json::Value & json)
{
	IThing::Deserialize( json );

	if( json.isMember("m_Name") )
		m_Name = json["m_Name"].asString();
	if( json.isMember("m_Info") )
		m_Info = json["m_Info"].asString();
	if( json.isMember("m_Confidence") )
		m_Confidence = json["m_Confidence"].asDouble();
	if( json.isMember("m_Threshold") )
		m_Threshold = json["m_Threshold"].asDouble();
}

