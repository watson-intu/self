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


#ifndef SELF_MICROPHONE_H
#define SELF_MICROPHONE_H

#include <list>

#include "AudioData.h"
#include "sensors/ISensor.h"
#include "SelfLib.h"

//! This is the base class for a standard microphone, this is not a concrete implementation. For that
//! see DSMicrophone or NaoMicrophone
class SELF_API Microphone : public ISensor
{
public:
	RTTI_DECL();

	//! Construction
	Microphone() : ISensor("Microphone"), 
		m_RecordingHZ( 16000 ),
		m_RecordingBits( 16 ), 
		m_RecordingChannels( 1 ),
		m_BinaryType( "audio/L16;rate=16000;channels=1" )
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! ISensor interface
	virtual const char * GetDataType()
	{
		return AudioData::GetStaticRTTI().GetName().c_str();
	}
	virtual const char * GetBinaryType()
	{
		return m_BinaryType.c_str();
	}

	virtual bool OnStart();
	virtual bool OnStop();
	virtual void OnPause();
	virtual void OnResume();

protected:
	//! Data
	int m_RecordingHZ;
	int m_RecordingBits;
	int m_RecordingChannels;
	std::string m_BinaryType;
};

#endif
