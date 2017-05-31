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


#ifndef TELE_SPEECH_GESTURE_H
#define TELE_SPEECH_GESTURE_H

#include "gestures/SpeechGesture.h"


class Sound;
class ITextToSpeech;
class ITelephony;
struct Voices;

//! This gesture wraps the local speech synthesis so the self can speak
class SELF_API TelephonySpeechGesture : public SpeechGesture
{
public:

	//! Types
	typedef std::vector< IGesture::SP >		OverrideList;
	typedef boost::shared_ptr<TelephonySpeechGesture>	SP;
	typedef boost::weak_ptr<TelephonySpeechGesture>	WP;

	RTTI_DECL();

	//! Construction
	TelephonySpeechGesture() : m_pTTS(NULL), m_pVoices(NULL), m_SampleRate(16000), SpeechGesture("tts")
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IGesture interface
	virtual bool Start();
	virtual bool Stop();
	virtual bool Execute(GestureDelegate a_Callback, const ParamsMap & a_Params);
	virtual bool Abort();


private:
	void StartSpeech();
	void OnVoices(Voices * a_pVoices);
	void OnSpeechData(Sound * a_pSound);
	void WaitOnBuffer();
	void OnSpeechDone();
	void ParseAudioData(const std::string & a_Data);

	ITextToSpeech *		m_pTTS;
	ITelephony *        m_pTelephony;
	Voices *			m_pVoices;
	bool				m_bOverride;
	int 				m_SampleRate;
	OverrideList		m_Overrides;
};


#endif //TELE_SPEECH_GESTURE_H