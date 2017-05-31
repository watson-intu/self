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


#include "WeatherIntent.h"
#include "services/ILanguageParser.h"

REG_SERIALIZABLE(WeatherIntent);
RTTI_IMPL( WeatherIntent, IIntent );

void WeatherIntent::Serialize(Json::Value &json)
{
    IIntent::Serialize(json);
	json["m_Location"] = m_Location;
	json["m_Parse"] = m_Parse;
}

void WeatherIntent::Deserialize(const Json::Value &json)
{
    IIntent::Deserialize(json);
	if( json.isMember("m_Location") )
		m_Location = json["m_Location"].asString();
	if( json.isMember("m_Parse") )
		m_Parse = json["m_Parse"];
}

void WeatherIntent::Create(const Json::Value & a_Intent, const Json::Value & a_Parse)
{
	IIntent::Create(a_Intent, a_Parse);
	if (a_Parse.isMember("Entities"))
		m_Parse = a_Parse["Entities"];
    if ( a_Intent.isMember("confidence") )
    	m_Confidence = a_Intent["confidence"].asDouble();

	ILanguageParser::FindCity( m_Parse, m_Location );

	if (a_Intent.isMember("conversation") && a_Intent["conversation"].isMember("entities") )
	{
		Json::Value conversationEntities = a_Intent["conversation"]["entities"];
		for(size_t i=0;i<conversationEntities.size();++i)
		{
			if (conversationEntities[i].isMember("entity") && conversationEntities[i]["entity"] == "sys-date" )
			{
				m_Date = conversationEntities[i]["value"].asString();
				break;
			}
		}
	}

	Log::Debug("WeatherIntent", "Parse is %s", m_Parse.toStyledString().c_str());
}