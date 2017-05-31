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


#define ENABLE_CONTENT_SERIALIZATION		1

#include "Image.h"
#include "utils/StringUtil.h"

REG_SERIALIZABLE(Image);
RTTI_IMPL( Image, IThing );

void Image::Serialize(Json::Value &json) 
{
	IThing::Serialize( json );

#if ENABLE_CONTENT_SERIALIZATION
	json["m_Content"] = StringUtil::EncodeBase64( m_Content );
#endif
	json["m_Origin"] = m_Origin;
}

void Image::Deserialize(const Json::Value &json) 
{
	IThing::Deserialize( json );

#if ENABLE_CONTENT_SERIALIZATION
	if ( json.isMember("m_Content") )
		m_Content = StringUtil::DecodeBase64( json["m_Content"].asString() );
#endif
	if ( json["m_Origin"].isString() )
		m_Origin = json["m_Origin"].asString();
}
