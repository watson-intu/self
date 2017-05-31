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


#ifndef SELF_DISKAUDIOSENSOR_H
#define SELF_DISKAUDIOSENSOR_H

#include "sensors/Microphone.h"

class SELF_API DiskAudioSensor : public Microphone
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<DiskAudioSensor>		SP;
	typedef boost::weak_ptr<DiskAudioSensor>		WP;

	//! Construction
	DiskAudioSensor();
	~DiskAudioSensor();

	//! ISerializable interface
	void Serialize(Json::Value &json);
	void Deserialize(const Json::Value &json);

	//! ISensor interface
	virtual const char * GetDataType()
	{
		return "AudioData";
	}

	const std::string & GetRecordingsPath() const
	{
		return m_RecordingsPath;
	}


	bool GetCaptureStopped()
	{
		return m_CaptureStopped;
	}

	virtual bool OnStart();
	virtual bool OnStop();
	virtual void OnPause();
	virtual void OnResume();

private:
	//! Data
	volatile bool m_CaptureAudio;
	volatile bool m_Paused;
	volatile bool m_CaptureStopped;

	std::string m_RecordingsPath;
	bool m_Error;
	void *m_CaptureEvents[2];

	void CaptureAudio(void *);
	void SendAudio(AudioData *a_pData);
};

#endif //SELF_DISKAUDIOSENSOR_H
