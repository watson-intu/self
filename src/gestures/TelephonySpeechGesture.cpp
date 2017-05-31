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


#include "TelephonySpeechGesture.h"
#include "sensors/AudioData.h"
#include "services/ITextToSpeech.h"
#include "services/ITelephony.h"
#include "skills/SkillManager.h"
#include "utils/ThreadPool.h"
#include "utils/Sound.h"
#include "GestureManager.h"

#include "SelfInstance.h"

REG_SERIALIZABLE(TelephonySpeechGesture);
RTTI_IMPL( TelephonySpeechGesture, SpeechGesture );

bool TelephonySpeechGesture::Start()
{
	Log::Debug("TelephonySpeechGesture", "Starting...");

	if (! SpeechGesture::Start() )
		return false;

	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );

	// if overrides is true, then we find any existing gestures with the same
	// ID and pull them from the gesture manager. We will restore those gestures
	// if we are stopped.
	GestureManager * pManager = pInstance->GetGestureManager();
	assert( pManager != NULL );

	if ( pManager->FindGestures( m_GestureId, m_Overrides ) )
	{
		for( size_t i=0;i<m_Overrides.size();++i)
			m_Overrides[i]->AddOverride();
	}

	m_pTTS = SelfInstance::GetInstance()->FindService<ITextToSpeech>();
	if ( m_pTTS == NULL )
	{
		Log::Error( "TelephonySpeechGesture", "ITextToSpeech service is missing." );
		return false;
	}

	m_pTelephony = SelfInstance::GetInstance()->FindService<ITelephony>();
	if ( m_pTelephony == NULL )
	{
		Log::Error( "TelephonySpeechGesture", "ITelephony service is missing." );
		return false;
	}

	ParseAudioData(m_pTelephony->GetAudioOutFormat());
	m_pTTS->GetVoices( DELEGATE( TelephonySpeechGesture, OnVoices, Voices *, this ) );
	return true;
}

bool TelephonySpeechGesture::Stop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );

	// restore overridden gestures..
	if ( m_Overrides.size() > 0 )
	{
		GestureManager * pManager = pInstance->GetGestureManager();
		assert( pManager != NULL );

		for( size_t i=0;i<m_Overrides.size();++i)
			m_Overrides[i]->RemoveOverride();
		m_Overrides.clear();
	}

	return true;
}

void TelephonySpeechGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize( json );
}

void TelephonySpeechGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize( json );
}

bool TelephonySpeechGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	if ( PushRequest( a_Callback, a_Params ) )
		StartSpeech();

	return true;
}


bool TelephonySpeechGesture::Abort()
{
	Log::Debug("TelephonySpeechGesture", "Attempting to abort speech!");
	return false;
}

void TelephonySpeechGesture::OnVoices( Voices * a_pVoices )
{
	m_pVoices = a_pVoices;
	if ( ActiveRequest() != NULL )
	{
		StartSpeech();
	}
}

void TelephonySpeechGesture::StartSpeech()
{
	if ( m_pVoices != NULL )
	{
		bool bSuccess = false;

		Request * pReq = ActiveRequest();

		const std::string & text = pReq->m_Params["text"].asString();
		const std::string & gender = pReq->m_Params["gender"].asString();
		const std::string & language = pReq->m_Params["language"].asString();

		std::string voice = "en-US_MichaelVoice";
		for(size_t i=0;i<m_pVoices->m_Voices.size();++i)
		{
			Voice & v = m_pVoices->m_Voices[i];
			if ( v.m_Language == language && v.m_Gender == gender )
			{
				voice = v.m_Name;
				break;
			}
		}

		ITextToSpeech * pTTS = SelfInstance::GetInstance()->FindService<ITextToSpeech>();
		if ( pTTS != NULL )
		{
			// call the service to get the audio data for playing ..
			pTTS->SetVoice(voice);
			pTTS->ToSound( text, DELEGATE( TelephonySpeechGesture, OnSpeechData, Sound *, this ) );
			bSuccess = true;
		}
		else
			Log::Error( "TelephonySpeechGesture", "No ITextToSpeech service available." );

		if (! bSuccess )
			OnSpeechDone();
	}
}

void TelephonySpeechGesture::OnSpeechData( Sound * a_pSound )
{
	if (a_pSound != NULL)
	{
		Log::Debug("TelephonySpeechGesture", "OnSpeechData...sending to Telephony");

		if ( a_pSound->GetRate() != m_SampleRate )
			a_pSound->Resample(m_SampleRate);
		m_pTelephony->SendAudioIn(a_pSound->GetWaveData());

		delete a_pSound;
	}

	OnSpeechDone();
}

void TelephonySpeechGesture::OnSpeechDone()
{
	SelfInstance::GetInstance()->GetSensorManager()->ResumeSensorType(AudioData::GetStaticRTTI().GetName());

	// start the next speech if we have any..
	if ( PopRequest() )
		StartSpeech();
}

void TelephonySpeechGesture::ParseAudioData(const std::string & a_Data)
{
	std::vector<std::string> parts;
	StringUtil::Split( a_Data, ";", parts );
	if (parts.size() > 1 && StringUtil::StartsWith( parts[1], "rate=", true ))
	{
		std::vector<std::string> rate;
		StringUtil::Split( parts[1], "=", rate );
		if (rate.size() > 1)
			m_SampleRate = atoi( rate[1].c_str() );
	}
}

