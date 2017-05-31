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


#include "NameIntent.h"
#include "utils/StringUtil.h"

REG_SERIALIZABLE(NameIntent);
RTTI_IMPL(NameIntent, IIntent);

void NameIntent::Serialize(Json::Value &json)
{
	IIntent::Serialize(json);
}

void NameIntent::Deserialize(const Json::Value &json)
{
	IIntent::Deserialize(json);
}

void NameIntent::Create(const Json::Value & a_Intent, const Json::Value & a_Parse)
{
	IIntent::Create(a_Intent, a_Parse);
	if (a_Parse.isMember("Entities"))
		m_Parse = a_Parse["Entities"];

	if (a_Intent.isMember("confidence"))
		m_Confidence = a_Intent["confidence"].asDouble();

	Log::Debug("NameIntent", "m_Parse: %s", m_Parse.toStyledString().c_str());
	Log::Debug("NameIntent", "a_Intent: %s", a_Intent.toStyledString().c_str());
	if (m_Parse.isMember("entities"))
	{
		const std::string type = m_Parse["entities"][0]["type"].asString();
		if (type == "Person")
		{
			m_Name = m_Parse["entities"][0]["text"].asString();
		}
	}
	
	if (a_Intent["conversation"]["entities"].size() > 0)
	{
		Json::Value entities = a_Intent["conversation"]["entities"];
		for (size_t i = 0; i < entities.size(); ++i)
		{
			if (entities[i]["entity"] == "Possessive")
				m_Possessive = entities[i]["value"].asString();
		}
	}

	// handle non-proper names or lack of NLU..
	if ( m_Name.empty() && a_Intent["text"].isString() )
	{
		std::vector<std::string> words;
		StringUtil::Split( a_Intent["text"].asString(), " ", words);

		if ( words.size() >= 4 )	// "your name is X" or "my name is X"
			m_Name = words[words.size() - 1];

	}
}