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


#include "SpeechGesture.h"

REG_SERIALIZABLE( SpeechGesture );
RTTI_IMPL( SpeechGesture, IGesture );

void SpeechGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize( json );
}

void SpeechGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize( json );
}

bool SpeechGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	Log::Debug( "SpeechGesture", "Execute speech gesture, text = %s", a_Params["text"].asCString() );
	if ( a_Callback.IsValid() )
		a_Callback( this );
	return true;
}

bool SpeechGesture::Abort()
{
	Log::Debug("SpeechGesture", "Attempting to abort speech!");
	return false;
}

