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


#include "PostureGesture.h"

REG_SERIALIZABLE( PostureGesture );
RTTI_IMPL( PostureGesture, IGesture );

void PostureGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize( json );
	json["m_PostureId"] = m_PostureId;
	json["m_fSpeed"] = m_fSpeed;
	json["m_bDisableFallManager"] = m_bDisableFallManager;
}

void PostureGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize( json );
	if ( json.isMember( "m_PostureId" ) )
		m_PostureId = json["m_PostureId"].asString();
	if ( json.isMember( "m_fSpeed" ) )
		m_fSpeed = json["m_fSpeed"].asFloat();
	if ( json.isMember( "m_bDisableFallManager" ) )
		m_bDisableFallManager = json["m_bDisableFallManager"].asBool();
}

bool PostureGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	Log::Status( "PostureGesture", "Execute posture gesture (%s).", m_PostureId.c_str() );
	if( a_Callback.IsValid() )
		a_Callback( this );
	return true;
}

bool PostureGesture::Abort()
{
	return false;
}

