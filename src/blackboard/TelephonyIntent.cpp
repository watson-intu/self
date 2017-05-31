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


#include "TelephonyIntent.h"

REG_SERIALIZABLE(TelephonyIntent);
RTTI_IMPL( TelephonyIntent, IIntent );

void TelephonyIntent::Serialize(Json::Value &json)
{
	IIntent::Serialize(json);

	json["m_Text"] = m_Text;
	json["m_TelephonyAction"] = m_TelephonyAction;
	json["m_ToNumber"] = m_ToNumber;
}

void TelephonyIntent::Deserialize(const Json::Value &json)
{
	IIntent::Deserialize(json);

	if ( json.isMember( "m_Text" ) )
		m_Text = json["m_Text"].asString();
	if ( json.isMember( "m_TelephonyAction" ) )
		m_TelephonyAction = json["m_TelephonyAction"].asString();
	if ( json.isMember( "m_ToNumber" ) )
		m_ToNumber = json["m_ToNumber"].asString();
}

void TelephonyIntent::Create(const Json::Value & a_Intent, const Json::Value & a_Parse)
{
	IIntent::Create(a_Intent, a_Parse);
	if ( a_Intent.isMember("text") )
		m_Text = a_Intent["text"].asString();

	m_TelephonyAction = "INITIATING";
}

