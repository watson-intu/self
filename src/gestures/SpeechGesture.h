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


#ifndef SPEECH_GESTURE_H
#define SPEECH_GESTURE_H

#include "IGesture.h"
#include "SelfLib.h"

//! This gesture wraps the local speech sythesis so the self can speak
class SELF_API SpeechGesture : public IGesture
{
public:
	RTTI_DECL();

	//! Construction
	SpeechGesture() 
	{}

	SpeechGesture(const std::string & a_gestureId) : IGesture(a_gestureId)
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IGesture interface
	virtual bool Execute( GestureDelegate a_Callback, const ParamsMap & a_Params );
	virtual bool Abort();
};


#endif //SPEECH_GESTURE_H
