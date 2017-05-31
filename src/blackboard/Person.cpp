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


#include "Person.h"
#include "SelfInstance.h"
#include "utils/UniqueID.h"

REG_SERIALIZABLE( Person );
RTTI_IMPL( Person, IThing );

Person::Person() : IThing( TT_PERCEPTION )
{}

void Person::Serialize(Json::Value &json) 
{
	IThing::Serialize( json );

	json["m_Gender"] = m_Gender;
	json["m_AgeRange"] = m_AgeRange;
	json["m_Origin"] = m_Origin;
	SerializeVector( "m_FaceLocation", m_FaceLocation, json );
	json["m_FaceImage"] = StringUtil::EncodeBase64( m_FaceImage );
}

void Person::Deserialize(const Json::Value &json) 
{
	IThing::Deserialize( json );

	if ( json.isMember( "m_Gender" ) )
		m_Gender = json["m_Gender"].asString();
	if ( json.isMember( "m_AgeRange" ) )
		m_AgeRange = json["m_AgeRange"].asString();
	if (json.isMember("m_Origin"))
		m_Origin = json["m_Origin"].asString();
	DeserializeVector( "m_FaceLocation", json, m_FaceLocation );
	if (json["m_FaceImage"].isString())
		m_FaceImage = StringUtil::DecodeBase64(json["m_FaceImage"].asString());
}

