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


#include "DuplicateFilter.h"

REG_SERIALIZABLE(DuplicateFilter);
RTTI_IMPL(DuplicateFilter, IClassFilter);

DuplicateFilter::DuplicateFilter() :
	m_LastIntentTime(0.0),
	m_MinIntentWindow(2.0)
{}

void DuplicateFilter::Serialize(Json::Value & json)
{
	json["m_MinIntentWindow"] = m_MinIntentWindow;
}

void DuplicateFilter::Deserialize(const Json::Value & json)
{
	if (json.isMember("m_MinIntentWindow"))
		m_MinIntentWindow = json["m_MinIntentWindow"].asDouble();
}

bool DuplicateFilter::ApplyFilter(Json::Value & a_Intent)
{
	const std::string & top_class = a_Intent["top_class"].asString();

	Time now;
	if ( m_LastIntent != top_class
		|| (now.GetEpochTime() - m_LastIntentTime) > m_MinIntentWindow)
	{
		m_LastIntent = top_class;
		m_LastIntentTime = now.GetEpochTime();
		return false;
	}

	return true;
}


