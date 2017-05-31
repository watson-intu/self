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


#include "IntentFilter.h"

REG_SERIALIZABLE(IntentFilter);
RTTI_IMPL(IntentFilter, IClassFilter);

IntentFilter::IntentFilter() 
{}

void IntentFilter::Serialize(Json::Value & json)
{
	SerializeVector( "m_Intents", m_Intents, json );
}

void IntentFilter::Deserialize(const Json::Value & json)
{
	DeserializeVector( "m_Intents", json, m_Intents );
}

bool IntentFilter::ApplyFilter(Json::Value & a_Intent)
{
	const std::string & top_class = a_Intent["top_class"].asString();

	for(size_t i=0;i<m_Intents.size();++i)
		if ( top_class.compare( m_Intents[i] ) == 0 )
			return true;

	return false;
}


