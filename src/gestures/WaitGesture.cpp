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


#include "WaitGesture.h"
#include "utils/TimerPool.h"
#include "SelfInstance.h"

REG_SERIALIZABLE( WaitGesture );
RTTI_IMPL(WaitGesture, IGesture);

void WaitGesture::Serialize(Json::Value & json)
{
    IGesture::Serialize( json );
    json["m_DelayInterval"] = m_fDelayInterval;
}

void WaitGesture::Deserialize(const Json::Value & json)
{
    IGesture::Deserialize( json );
    m_fDelayInterval = json["m_DelayInterval"].asFloat();
}

bool WaitGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
    Log::Debug( "WaitGesture", "Execute wait gesture" );
    m_Callback = a_Callback;

	m_spWaitTimer = TimerPool::Instance()->StartTimer(
		VOID_DELEGATE( WaitGesture, TerminateExecution, this ), m_fDelayInterval, true, false );

    return true;
}

bool WaitGesture::Abort()
{
    return false;
}

void WaitGesture::TerminateExecution()
{
    Log::Debug("WaitGesture", "Terminating wait gesture");

    if(m_Callback.IsValid())
        m_Callback(this);
	m_spWaitTimer.reset();
}