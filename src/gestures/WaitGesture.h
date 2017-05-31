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


#ifndef SELF_WAITGESTURE_H
#define SELF_WAITGESTURE_H

#include "utils/TimerPool.h"
#include "IGesture.h"
#include "SelfLib.h"

//! This gesture waits for a specified period of time
class SELF_API WaitGesture : public IGesture
{
public:
	RTTI_DECL();

	//! Construction
	WaitGesture()
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IGesture interface
	virtual bool Execute(GestureDelegate a_Callback, const ParamsMap & a_Params);
	virtual bool Abort();

private:

	//! TimerPool callback
	void TerminateExecution();

	//! Data
	GestureDelegate m_Callback;
	float m_fDelayInterval;
	TimerPool::ITimer::SP m_spWaitTimer;
};

#endif //SELF_WAITGESTURE_H