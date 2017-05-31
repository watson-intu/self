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


#include "Microphone.h"

REG_SERIALIZABLE( Microphone );
RTTI_IMPL(Microphone, ISensor);

void Microphone::Serialize(Json::Value & json)
{
	ISensor::Serialize( json );

	json["m_RecordingHZ"] = m_RecordingHZ;
	json["m_RecordingBits"] = m_RecordingBits;
	json["m_RecordingChannels"] = m_RecordingChannels;
}

void Microphone::Deserialize(const Json::Value & json)
{
	ISensor::Deserialize( json );

	if ( json.isMember("m_RecordingHZ") )
		m_RecordingHZ = json["m_RecordingHZ"].asInt();
	if ( json.isMember("m_RecordingBits") )
		m_RecordingBits = json["m_RecordingBits"].asInt();
	if ( json.isMember("m_RecordingChannels") )
		m_RecordingChannels = json["m_RecordingChannels"].asInt();

	m_BinaryType = StringUtil::Format( "audio/L%u;rate=%u;channels=%u", m_RecordingBits, m_RecordingHZ, m_RecordingChannels );
}

bool Microphone::OnStart()
{
	Log::Warning( "Microphone", "Microphone is not implemented." );
	return true;
}

bool Microphone::OnStop()
{
	return true;
}

void Microphone::OnPause()
{}

void Microphone::OnResume()
{}
