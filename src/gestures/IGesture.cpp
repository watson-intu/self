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


#include "IGesture.h"

RTTI_IMPL( IGesture, ISerializable );

void IGesture::Serialize(Json::Value & json)
{
	json["m_bEnabled"] = m_bEnabled;
	json["m_GestureId"] = m_GestureId;
}

void IGesture::Deserialize(const Json::Value & json)
{
	if ( json["m_bEnabled"].isBool() )
		m_bEnabled = json["m_bEnabled"].asBool();
	if ( json["m_GestureId"].isString() )
		m_GestureId = json["m_GestureId"].asString();
}

bool IGesture::Start()
{
	return true;
}

bool IGesture::Stop()
{
	return true;
}

bool IGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	Log::Debug( "IGesture", "Gesture %s Execute not implemented.", m_GestureId.c_str() );
	if( a_Callback.IsValid() )
		a_Callback( this );
	return true;
}

bool IGesture::Abort()
{
	return false;
}

