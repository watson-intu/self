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


#define ENABLE_DEBUGGING			0

#include "LearningIntent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"

REG_SERIALIZABLE(LearningIntent);
RTTI_IMPL( LearningIntent, IIntent );

void LearningIntent::Serialize(Json::Value &json) 
{
	IIntent::Serialize(json);

    json["m_Text"] = m_Text;
    json["m_Target"] = m_Target;
    json["m_Verb"] = m_Verb;
}

void LearningIntent::Deserialize(const Json::Value &json) 
{
	//Log::Debug( "LearningIntent", "Deserialize: %s", json.toStyledString().c_str() );

	IIntent::Deserialize(json);

	m_Text = json["m_Text"].asString();
	m_Target = json["m_Target"].asString();
	m_Verb = json["m_Verb"].asString();
}

void LearningIntent::Create(const Json::Value & a_Intent, const Json::Value & a_Parse)
{
	IIntent::Create(a_Intent, a_Parse);
	if (a_Parse.isMember("POS"))
		m_Parse = a_Parse["POS"];
	if ( a_Intent.isMember("text") )
		m_Text = a_Intent["text"].asString();
	if ( a_Intent.isMember("confidence") )
		m_Confidence = a_Intent["confidence"].asDouble();
	if (m_Parse.isMember( "sentences" ) )
		m_Tags = m_Parse["sentences"][0]["tags"];

	const std::string & top_class = a_Intent["top_class"].asString();

#if ENABLE_DEBUGGING
	Log::Debug("LearningIntent", "m_TextParse: %s", m_Tags.toStyledString().c_str());
#endif
	if (top_class == "positive_feedback" || top_class == "negative_feedback")
	{
		m_Verb = "feedback";
		m_Target = top_class;
		if ( a_Intent.isMember("goal_params") )
			m_Answer = a_Intent["goal_params"]["answer"]["response"][0].asString();
	}
	else if (top_class == "learn_question" 
		|| top_class == "learn_statement"
		|| top_class == "learn_command" )
	{
		m_Verb = top_class;
	}
	else if ( top_class == "forget_skill" )
	{
		m_Verb = top_class;
		for(size_t i=0;i<m_Tags.size();++i)
		{
			const std::string & tag = m_Tags[i]["tag"].asString();
			const std::string & text = m_Tags[i]["text"].asString();

			if ( (tag == "VB" || tag == "VBP") && text != "forget" )
			{
				m_Target = text;
				break;
			}
			else if ( tag == "NN" && m_Target.size() == 0 )
				m_Target = text;
		}
	}
	else if (top_class == "learn_skill")
	{
		m_Verb = top_class;
		for(size_t i=0;i<m_Tags.size();++i)
		{
			const std::string & tag = m_Tags[i]["tag"].asString();
			const std::string & text = m_Tags[i]["text"].asString();

			if ( (tag == "VB" || tag == "VBP") && text != "forget" )
			{
				m_Target = text;
				break;
			}
			else if ( tag == "NN" && m_Target.size() == 0 )
				m_Target = text;
		}
	}
	else if (top_class == "learn_object" || top_class == "forget_object" )
	{
		m_Verb = top_class;

		for(size_t i=0;i<m_Tags.size();++i)
		{
			const std::string & tag = m_Tags[i]["tag"].asString();
			const std::string & text = m_Tags[i]["text"].asString();

			if ( tag == "NN" || tag == "NNP" )
			{
				if ( m_Target.size() > 0 )
					m_Target += " ";
				m_Target += text;
			}
		}
		// This will allow the recognized object to be added to Conversation as a context variable
		PublishObjectRecognized();
	}
	else if (top_class == "learn_schedule")
	{
		m_Verb = top_class;
		m_Target = FindTags( "CD" );
	}
	else if (top_class == "learn_number" )
	{
		m_Verb = top_class;
		std::string number;

		// iterate through conversation entities and extract all numbers, append to string
		if (a_Intent.isMember("conversation") && a_Intent["conversation"].isMember("entities") )
		{
			Json::Value conversationEntities = a_Intent["conversation"]["entities"];
			for(size_t i=0;i<conversationEntities.size();++i)
			{
				if (conversationEntities[i].isMember("entity") && conversationEntities[i]["entity"] == "sys-number" )
					number += conversationEntities[i]["value"].asString();
			}
		}

		// todo: Build this out to handle any sort of phone number, including international
		// This currently only works for North American numbers, ie: 1 (555) 555-5555

		// if phone number is missing international code 1
		if( number.size() == 10 )
			number = "1" + number;

		// if phone number matches phone number format
		if( number.size() == 11 )
			m_PhoneNumber = number;
	}
}

std::string LearningIntent::FindTags( const std::string & a_Tag )
{
	std::string tags;
	for(size_t i=0;i<m_Tags.size();++i)
	{
		const std::string & tag = m_Tags[i]["tag"].asString();
		const std::string & text = m_Tags[i]["text"].asString();

		if ( tag == a_Tag )
		{
			if ( tags.size() > 0 )
				tags += " ";
			tags += text;
		}
	}

	return tags;
}


void LearningIntent::PublishObjectRecognized()
{
	if( !m_Target.empty() )
	{
		AddChild( IThing::SP(
				new IThing(TT_PERCEPTION, "RecognizedObject", IThing::JsonObject("m_Object", m_Target), 3600.0f )));
	}
}

