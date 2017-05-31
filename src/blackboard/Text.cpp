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


#include "Text.h"

REG_SERIALIZABLE( Text );
RTTI_IMPL( Text, IThing );

void Text::Serialize(Json::Value &json) 
{
	IThing::Serialize( json );

	json["m_Text"] = m_Text;
	json["m_fConfidence"] = m_fConfidence;
	json["m_LocalDialog"] = m_LocalDialog;
	json["m_ClassifyIntent"] = m_ClassifyIntent;
	json["m_Language"] = m_Language;
}

void Text::Deserialize(const Json::Value &json) 
{
	IThing::Deserialize( json );

	if ( json.isMember( "m_Text" ) )
		m_Text = json["m_Text"].asString();
	if ( json.isMember( "m_fConfidence" ) )
		m_fConfidence = json["m_fConfidence"].asDouble();
	if ( json.isMember( "m_LocalDialog" ) )
		m_LocalDialog = json["m_LocalDialog"].asBool();
	if ( json.isMember( "m_ClassifyIntent" ) )
		m_ClassifyIntent = json["m_ClassifyIntent"].asBool();
	if ( json.isMember( "m_Language" ) )
		m_Language = json["m_Language"].asString();
}