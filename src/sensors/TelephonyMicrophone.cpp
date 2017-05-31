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


#include "TelephonyMicrophone.h"
#include "SelfInstance.h"
#include "utils/Log.h"
#include "utils/TimerPool.h"
#include "utils/SelfException.h"

#include <stdio.h>

REG_SERIALIZABLE( TelephonyMicrophone );
RTTI_IMPL(TelephonyMicrophone, Microphone);

void TelephonyMicrophone::Serialize(Json::Value & json)
{
	Microphone::Serialize( json );
}

void TelephonyMicrophone::Deserialize(const Json::Value & json)
{
	Microphone::Deserialize( json );
}

bool TelephonyMicrophone::OnStart()
{
	Log::Status("TelephonyMicrophone", "TelephonyMicrophone started");
	return true;
}

bool TelephonyMicrophone::OnStop()
{
	// stop our streaming thread..
	Log::Status("TelephonyMicrophone", "TelephonyMicrophone stopped");
	return true;
}

void TelephonyMicrophone::SendAudioData( const std::string & a_Data )
{
	ThreadPool::Instance()->InvokeOnMain<AudioData *>(DELEGATE(TelephonyMicrophone, SendingData, AudioData *, this),
	                                                  new AudioData(a_Data, m_RecordingHZ, 1, m_RecordingBits));
}

void TelephonyMicrophone::SendingData(AudioData * a_pData)
{
	SendData(a_pData);
}

void TelephonyMicrophone::OnPause()
{}

void TelephonyMicrophone::OnResume()
{}
