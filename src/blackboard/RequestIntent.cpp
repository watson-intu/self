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


#include "RequestIntent.h"
#include "SelfInstance.h"
#include "utils/StringUtil.h"

#define ENABLE_DEBUGGING			0

REG_SERIALIZABLE(RequestIntent);
RTTI_IMPL( RequestIntent, IIntent );

void RequestIntent::Serialize(Json::Value &json) 
{
	IIntent::Serialize(json);

    json["m_Text"] = m_Text;
	json["m_Tags"] = m_Tags;
    json["m_Type"] = m_Type;

	Json::Value & requests = json["m_Requests"];
    for(size_t i = 0; i < m_Requests.size(); ++i)
    {
        requests[i]["m_Verb"] = m_Requests[i].m_Verb;
		requests[i]["m_Target"] = m_Requests[i].m_Target;
    }
}

void RequestIntent::Deserialize(const Json::Value &json) 
{
	IIntent::Deserialize(json);

	m_Text = json["m_Text"].asString();
	m_Tags = json["m_Tags"];
	m_Type = json["m_Type"].asString();

	const Json::Value & requests = json["m_Requests"];
	for (Json::ValueConstIterator iReq = requests.begin(); iReq != requests.end(); ++iReq)
	{
		const Json::Value & req = *iReq;
		m_Requests.push_back(Request(req["m_Verb"].asString(), req["m_Target"].asString()));
	}
}

void RequestIntent::Create(const Json::Value & a_Intent, const Json::Value & a_Parse)
{
	IIntent::Create(a_Intent, a_Parse);
	IGraph::SP spGraph = SelfInstance::GetInstance()->GetKnowledgeGraph();
	assert(spGraph.get() != NULL);

	if (a_Parse.isMember("POS"))
		m_Parse = a_Parse["POS"];
	if ( a_Intent.isMember("text") )
		m_Text = a_Intent["text"].asString();
	if ( a_Intent.isMember("confidence") )
		m_Confidence = a_Intent["confidence"].asDouble();
	if( a_Intent.isMember("conversation") && a_Intent["conversation"].isMember("entities") )
		m_Entities = a_Intent["conversation"]["entities"];
	if ( m_Parse.isMember( "sentences" ) )
		m_Tags = m_Parse["sentences"][0]["tags"];

	const std::string & intent = a_Intent["top_class"].asString();

	// TODO: This is going away, all new requests have the "request" intent
	if (intent == "request_skill")
	{
		std::string verb, lastverb;
		for(size_t i=0;i<m_Tags.size();++i)
		{
			const std::string & tag = m_Tags[i]["tag"].asString();
			const std::string & text = m_Tags[i]["text"].asString();

			if ( tag == "VB" || (tag == "NN" && verb.size() == 0) )
			{
				verb = text;
			}
			else if ( tag == "NN" )
			{
				if ( verb == lastverb && m_Requests.size() > 0 )
				{
					// handle multi-part nouns (e.g. water bottle)
					m_Requests.back().m_Target += " ";
					m_Requests.back().m_Target += tag;
				}
				else
				{
					m_Requests.push_back( Request( verb, text) );
					lastverb = verb;
				}
			}
		}
		m_Type = "linear";	// TODO: not sure how to set this just yet, so defaulting to linear 
	}
	else if ( intent == "request_object" ) 
	{
		m_Requests.push_back( Request( "identify_object", "") );
		std::string previousTag = "";

		for(size_t i=0;i<m_Tags.size();++i)
		{
			const std::string &tag = m_Tags[i]["tag"].asString();
			if( tag == "PRP$" )
				m_bGetAllItems = true;
			else if( tag == "DT" && previousTag == "VBZ" ) // "is this"
				m_bGetAllItems = false; // potentially reset to false
			else if( tag == "DT" && previousTag == "VB" ) // "identify this"
				m_bGetAllItems = false; // potentially reset to false
			else if( tag == "VBZ" && previousTag == "DT" ) // "this is"
				m_bGetAllItems = false; // potentially reset to false
			else if( tag == "VB" && previousTag == "PRP" ) // "you see"
				m_bGetAllItems = true;

			previousTag = tag;
		}
	}
	else
	{
		m_Type = "linear";
		m_Requests.push_back(Request(intent, ""));
	}
}

