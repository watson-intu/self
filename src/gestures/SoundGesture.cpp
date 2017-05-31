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


#include "SoundGesture.h"

REG_SERIALIZABLE( SoundGesture );
RTTI_IMPL( SoundGesture, IGesture );

void SoundGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize( json );

	json["m_Sound"] = m_Sound;
	json["m_fVolume"] = m_fVolume;
	json["m_fPan"] = m_fPan;
}

void SoundGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize( json );

	if ( json.isMember( "m_Sound" ) )
		m_Sound = json["m_Sound"].asString();
	if ( json.isMember( "m_fVolume" ) )
		m_fVolume = json["m_fVolume"].asFloat();
	if ( json.isMember( "m_fPan" ) )
		m_fPan = json["m_fPan"].asFloat();
}

bool SoundGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	if ( a_Params.GetData().isMember("sound") )
		m_Sound = a_Params["sound"].asString();

	Log::Debug( "SoundGesture", "Playing sound %s", m_Sound.c_str() );
	if ( a_Callback.IsValid() )
		a_Callback( this );
	return true;
}

bool SoundGesture::Abort()
{
	return false;
}

