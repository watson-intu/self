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


#include "NewsIntent.h"

REG_SERIALIZABLE(NewsIntent);
RTTI_IMPL(NewsIntent, IIntent);

void NewsIntent::Serialize(Json::Value &json)
{
	IIntent::Serialize(json);
	json["m_Company"] = m_Company;
	json["m_Parse"] = m_Parse;
}

void NewsIntent::Deserialize(const Json::Value &json)
{
	IIntent::Deserialize(json);
	if( json.isMember("m_Company") )
		m_Company = json["m_Company"].asString();
	if( json.isMember("m_Parse") )
		m_Parse = json["m_Parse"];
}

void NewsIntent::Create(const Json::Value & a_Intent, const Json::Value & a_Parse)
{
	Log::Debug("NewsIntent", "Create(): %s", a_Parse.toStyledString().c_str());
	IIntent::Create(a_Intent, a_Parse);
	if (a_Parse.isMember("Entities"))
		m_Parse = a_Parse["Entities"];
	if (a_Intent.isMember("confidence"))
		m_Confidence = a_Intent["confidence"].asDouble();
	if (m_Parse.isMember("entities"))
	{
		const std::string entities = m_Parse["entities"][0]["type"].asString();
		if (entities == "Company" || entities == "City")
		{
			m_Company = m_Parse["entities"][0]["text"].asString();
		}
	}
	m_Parse = a_Parse;
}