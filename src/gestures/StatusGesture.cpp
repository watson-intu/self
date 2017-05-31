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


#include "StatusGesture.h"

REG_SERIALIZABLE( StatusGesture );
RTTI_IMPL( StatusGesture, IGesture );

void StatusGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize( json );

	json["m_Red"] = m_Red;
	json["m_Green"] = m_Green;
	json["m_Blue"] = m_Blue;

	std::map<std::string, std::string>::const_iterator it = m_StateMap.begin(), end = m_StateMap.end();
	for(; it != end; ++it)
	{
		Json::Value & stateMap = json["m_StateMap"];
		stateMap[it->first] = it->second;
	}
}

void StatusGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize( json );

	m_Red = json["m_Red"].asFloat();
	m_Green = json["m_Green"].asFloat();
	m_Blue = json["m_Blue"].asFloat();

	const Json::Value & stateMap = json["m_StateMap"];
	for(Json::ValueConstIterator iter = stateMap.begin(); iter != stateMap.end(); iter++)
	{
		m_StateMap.insert(std::pair<std::string, std::string>(iter.key().asString(), (*iter).asString()));
	}
}

bool StatusGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	Log::Debug( "StatusGesture", "Execute status gesture." );
	if ( a_Callback.IsValid() )
		a_Callback( this );
	return true;
}

bool StatusGesture::Abort()
{
	return false;
}

