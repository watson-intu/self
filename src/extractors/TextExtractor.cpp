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


//! Define to non-zero to enable refreshing web sockets each time we get a final transcription
#define ENABLE_STT_REFRESH			0

#include "TextExtractor.h"

#include "SelfInstance.h"
#include "sensors/SensorManager.h"
#include "sensors/ISensor.h"
#include "sensors/AudioData.h"
#include "sensors/TextData.h"
#include "skills/SkillManager.h"
#include "utils/Delegate.h"
#include "utils/JpegHelpers.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"
#include "blackboard/Health.h"
#include "blackboard/Failure.h"
#include "services/ISpeechToText.h"
#include "services/ILanguageTranslation.h"

#include <limits>

#undef max
#undef min

REG_SERIALIZABLE(TextExtractor);
RTTI_IMPL( TextExtractor, IExtractor );
RTTI_IMPL_BASE_EMBEDDED( TextExtractor, IAudioFilter);

//! TextExtractor Main
const float DEFAULT_CONFIDENCE_THRESHOLD = 0.3f;
const float DEFAULT_LOCAL_CONFIDENCE_THRESHOLD = 0.5f;

TextExtractor::TextExtractor() :
	m_FocusTimeout( 5.0f ),
	m_MinConfidence( DEFAULT_CONFIDENCE_THRESHOLD ),
	m_StdDevThreshold( 4.5f ),
	m_EnergyAverageSampleCount( 40 ),
	m_LastFailureResponse(Time().GetEpochTime()),
	m_MinFailureResponseInterval(7.0),
	m_EnergyTimeInterval(2.0),
	m_NameRecognizedThresholdDrop(0.1f),
	m_EnergyLevelCrossedTimestamp(0),
	m_FailureResponsesCount(0),
	m_MaxFailureResponsesCount(5),
	m_MinEnergyAvg(std::numeric_limits<float>::max()),
	m_MaxEnergyAvg(std::numeric_limits<float>::min()),
	m_NormalizedEnergyLevel(0),
	m_MaxConfidence(DEFAULT_CONFIDENCE_THRESHOLD + 0.2f),
	m_ConfidenceThreshold(DEFAULT_CONFIDENCE_THRESHOLD),
	m_ConfidenceThresholdLocal(DEFAULT_LOCAL_CONFIDENCE_THRESHOLD),
	m_BurnInCycles(0),
	m_Listening( true ),
	m_bStoreAudio(false),
	m_pAudioSensor( NULL ),
	m_nSpectrumSubs( 0 ),
	m_FFTHeight( 100 ),
	m_FFTWidth( 500 ),
	m_bAverageMax( true )
{
	//Give the TextExtractor fifteen seconds to gather background noise data
	//before we start to respond to failures
	m_LastFailureResponse += 15.0f;
}

TextExtractor::~TextExtractor()
{}

void TextExtractor::Serialize(Json::Value & json)
{
	IExtractor::Serialize( json );

	json["m_FocusTimeout"] = m_FocusTimeout;
	json["m_MinConfidence"] = m_MinConfidence;
	json["m_MinFailureResponseInterval"] = m_MinFailureResponseInterval;
	json["m_EnergyAverageSampleCount"] = m_EnergyAverageSampleCount;
	json["m_StdDevThreshold"] = m_StdDevThreshold;
	json["m_EnergyTimeInterval"] = m_EnergyTimeInterval;
	json["m_NormalizedEnergyAvg"] = m_NormalizedEnergyLevel;
	json["m_MaxConfidence"] = m_MaxConfidence;
	json["m_ConfidenceThreshold"] = m_ConfidenceThreshold;
	json["m_ConfidenceThresholdLocal"] = m_ConfidenceThresholdLocal;
	json["m_MaxFailureResponsesCount"] = m_MaxFailureResponsesCount;
	json["m_MinimumAge"] = m_MinimumAge;
	json["m_AgeTimeout"] = m_AgeTimeout;

	json["m_FFTHeight"] = m_FFTHeight;
	json["m_FFTWidth"] = m_FFTWidth;
	json["m_bAverageMax"] = m_bAverageMax;

	SerializeVector( "m_FailureResponses", m_FailureResponses, json );
	SerializeVector( "m_Filters", m_Filters, json );
}

void TextExtractor::Deserialize(const Json::Value & json)
{
	IExtractor::Deserialize( json );

	if ( json["m_FocusTimeout"].isNumeric() )
		m_FocusTimeout = json["m_FocusTimeout"].asFloat();
	if ( json.isMember("m_MinConfidence" ) )
		m_MinConfidence = json["m_MinConfidence"].asFloat();
	if ( json.isMember( "m_MinFailureResponseInterval" ) )
		m_MinFailureResponseInterval = json["m_MinFailureResponseInterval"].asDouble();
	if ( json.isMember( "m_EnergyAverageSampleCount" ) )
		m_EnergyAverageSampleCount = json["m_EnergyAverageSampleCount"].asInt();
	if ( json.isMember( "m_MaxFailureResponsesCount" ) )
		m_MaxFailureResponsesCount = json["m_MaxFailureResponsesCount"].asInt();
	if( json.isMember( "m_StdDevThreshold" ) )
		m_StdDevThreshold = json["m_StdDevThreshold"].asFloat();
	if( json.isMember( "m_EnergyTimeInterval" ) )
		m_EnergyTimeInterval = json["m_EnergyTimeInterval"].asFloat();
	if( json.isMember( "m_NormalizedEnergyAvg" ) )
		m_NormalizedEnergyLevel = json["m_NormalizedEnergyLevel"].asFloat();
	if( json.isMember( "m_MaxConfidence" ) )
		m_MaxConfidence = json["m_MaxConfidence"].asFloat();
	if( json.isMember( "m_ConfidenceThreshold" ) )
		m_ConfidenceThreshold = json["m_ConfidenceThreshold"].asFloat();
	if( json.isMember( "m_ConfidenceThresholdLocal" ) )
		m_ConfidenceThresholdLocal = json["m_ConfidenceThresholdLocal"].asFloat();
	if (json.isMember("m_MinimumAge"))
		m_MinimumAge = json["m_MinimumAge"].asInt();
	if (json.isMember("m_AgeTimeout"))
		m_AgeTimeout = json["m_AgeTimeout"].asFloat();

	if ( json["m_FFTHeight"].isNumeric() )
		m_FFTHeight = json["m_FFTHeight"].asInt();
	if ( json["m_FFTWidth"].isNumeric() )
		m_FFTWidth = json["m_FFTWidth"].asInt();
	if ( json["m_bAverageMax"].isBool() )
		m_bAverageMax = json["m_bAverageMax"].asBool();

	DeserializeVector( "m_FailureResponses", json, m_FailureResponses );
	DeserializeVector( "m_Filters", json, m_Filters );

	if ( m_FailureResponses.size() == 0 )
		m_FailureResponses.push_back( "Sorry, but I'm unable to recognize speech at the moment, please try again later." );
}

const char * TextExtractor::GetName() const
{
    return "TextExtractor";
}

bool TextExtractor::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert( pBlackboard != NULL );

	pBlackboard->SubscribeToType( "Health",
		DELEGATE( TextExtractor, OnHealth, const ThingEvent &, this ), TE_ADDED );
	pBlackboard->SubscribeToType( "Person",
		DELEGATE( TextExtractor, OnPerson, const ThingEvent &, this ), (ThingEventType)(TE_ADDED | TE_REMOVED) );

	m_spPerceptionRoot = pBlackboard->GetPerceptionRoot();
	m_spFocus = m_spPerceptionRoot;

	ITopics * pTopics = pInstance->GetTopics();
	assert( pTopics != NULL );

	pTopics->RegisterTopic( "conversation", "text/plain" );
	pTopics->RegisterTopic( "fft", "image/jpeg", DELEGATE( TextExtractor, OnSpectrumSubscribe, const ITopics::SubInfo &, this ) );

	pTopics->Subscribe( "conversation", DELEGATE( TextExtractor, OnConversation, const ITopics::Payload &, this ) );

	// find all audio sensors, then subscribe to them to receive their input..
	SensorManager * pSensorMgr = pInstance->GetSensorManager();
	assert( pSensorMgr != NULL );

	pSensorMgr->RegisterForSensor( "AudioData", 
		DELEGATE( TextExtractor, OnAddAudio, ISensor *, this ),
		DELEGATE( TextExtractor, OnRemoveAudio, ISensor *, this ) );

	// find all TextData sensors, then subscribe to them to receive their input..
	pSensorMgr->RegisterForSensor( "TextData",
		DELEGATE( TextExtractor, OnAddText, ISensor *, this ),
		DELEGATE( TextExtractor, OnRemoveText, ISensor *, this ) );

	return true;
}

bool TextExtractor::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;

	ITopics * pTopics = pInstance->GetTopics();
	assert( pTopics != NULL );

	pTopics->Unsubscribe( "conversation" );
	pTopics->UnregisterTopic( "conversation" );
	pTopics->UnregisterTopic( "fft" );

	ISpeechToText * pSTT = pInstance->FindService<ISpeechToText>();
	if ( pSTT != NULL && pSTT->IsListening() )
		pSTT->StopListening();

	if ( m_pAudioSensor != NULL )
	{
		m_pAudioSensor->Unsubscribe( this );
		m_pAudioSensor = NULL;
	}

	SensorManager * pSensorMgr = pInstance->GetSensorManager();
	assert( pSensorMgr != NULL );

	pSensorMgr->UnregisterForSensor( "AudioData", this );
	pSensorMgr->UnregisterForSensor( "TextData", this );

	pInstance->GetBlackBoard()->UnsubscribeFromType( "Health", this );

	Log::Status("TextExtractor", "TextExtractor stopped");
	return true;
}

void TextExtractor::OnRecognizeSpeech( RecognizeResults * a_pResults )
{
	SelfInstance * pInstance = SelfInstance::GetInstance();

	if( a_pResults != NULL )
    {
		for (size_t k = 0; k < a_pResults->m_Results.size(); ++k)
		{
			if (a_pResults->m_Results[k].m_Final)
			{
				const std::vector<SpeechAlt> & alternatives = a_pResults->m_Results[k].m_Alternatives;
				for (size_t i = 0; i < alternatives.size(); ++i)
				{
					const std::string & transcript = alternatives[i].m_Transcript;
					double confidence = alternatives[i].m_Confidence;

					Text::SP spText(new Text(transcript, confidence, false, true, a_pResults->GetLanguage()));

					Log::Status("Conversation", "----- Recognized Text: %s (%g/%g), Language: %s, TextId: %p",
						transcript.c_str(), confidence, m_ConfidenceThreshold, a_pResults->GetLanguage().c_str(), spText.get());
#if ENABLE_STT_REFRESH
					if(m_pSpeechToText != NULL)
						m_pSpeechToText->RefreshConnections();
#endif
					// check that our focus object is still attached to the blackboard, since Person objects may go away
					// without notice..
					if ( m_spFocus->GetBlackBoard() == NULL )
						m_spFocus = m_spPerceptionRoot;

					if (confidence > m_ConfidenceThreshold)
					{
						// check for difference in language between the heard language and the back-end language,
						// translated if needed.
						ILanguageTranslation * pTranslation = pInstance->FindService<ILanguageTranslation>();
						if (pTranslation != NULL && pTranslation->IsConfigured() && 
							a_pResults->GetLanguage() != pInstance->GetBackendLanguage() )
						{
							new TranslateReq( pTranslation, m_spFocus, spText, pInstance->GetBackendLanguage() );
						}
						else
						{
							pInstance->GetTopics()->Publish( "conversation", spText->GetText() );
							m_spFocus->AddChild( spText );
						}
					}
					else
					{
						m_spFocus->AddChild( Failure::SP(new Failure("Text", transcript, confidence, m_ConfidenceThreshold) ) );
					}
				}
			}
		}
		delete a_pResults;
	}
}

void TextExtractor::OnTextData(IData * data)
{
	TextData * pText = DynamicCast<TextData>(data);

	ISpeechToText * pSTT = SelfInstance::GetInstance()->FindService<ISpeechToText>();
	bool bConnected = pSTT != NULL && pSTT->IsConnected();

	Text::SP spText(new Text(pText->GetText(), pText->GetConfidence(), false, !bConnected));

	Log::Status("Conversation", "----- Received Text: %s (%g/%g), TextId: %p",
		pText->GetText().c_str(), pText->GetConfidence(), m_ConfidenceThresholdLocal, spText.get() );

	if (pText->GetConfidence() > m_ConfidenceThresholdLocal)
	{
		// NOTE: we only classify this type of intent if the speech to text service is non-functional.
		// TOOD: If this is keyboard input, then we probably want to always classify this type of intent.
		m_spFocus->AddChild(spText);

		if(m_FailureResponsesCount > 0)
			m_FailureResponsesCount = 0;
	}
	else if (!bConnected && m_FailureResponses.size() > 0
			 && m_EnergyLevelCrossedTimestamp + m_EnergyTimeInterval > Time().GetEpochTime())
	{
		//If the energy time-stamp (plus the window interval) is greater than the current time, then we know we likely have a legit
		//question for the robot, that we just don't have in the ASR
		m_FailureResponsesCount++;
		if(m_FailureResponsesCount == m_MaxFailureResponsesCount)
		{
			m_spFocus->AddChild( Say::SP( new Say( m_FailureResponses[ rand() % m_FailureResponses.size() ] ) ) );
			m_FailureResponsesCount = 0;
		}
	}
}

void TextExtractor::OnPerson( const ThingEvent & a_Event )
{
	Person::SP spPerson = DynamicCast<Person>( a_Event.GetIThing() );
	if ( spPerson )
	{
		if ( a_Event.GetThingEventType() == TE_ADDED )
		{
			// let us refocus on a new person if the current person is old enough..
			if ( m_spFocus && m_spFocus->GetAge() > m_FocusTimeout )
				m_spFocus = m_spPerceptionRoot;

			Person::SP spFocusPerson = DynamicCast<Person>( m_spFocus );
			if ( spFocusPerson )
			{
				if ( spPerson->FaceDistance() < spFocusPerson->FaceDistance() )
					m_spFocus = spPerson;
			}
			else
				m_spFocus = spPerson;
		}
		else if ( a_Event.GetThingEventType() == TE_REMOVED )
		{
			if ( m_spFocus == spPerson )
				m_spFocus = m_spPerceptionRoot;
		}
	}
}

void TextExtractor::OnHealth( const ThingEvent & a_Event )
{
	Health::SP spHealth = DynamicCast<Health>( a_Event.GetIThing() );
	if ( spHealth )
	{
		if ( spHealth->GetHealthName() == "RemoteNetwork" )
		{
			ISpeechToText * pSTT = SelfInstance::GetInstance()->FindService<ISpeechToText>();
			if ( pSTT != NULL && spHealth->IsError() )
			{
				Log::Error( "TextExtractor", "RemoteNetwork error, forcing disconnect on SpeechToText service." );
				pSTT->Disconnected();
			}
		}
	}
}

void TextExtractor::OnConversation( const ITopics::Payload & a_Payload )
{
	if ( a_Payload.m_RemoteOrigin[0] != 0 ) 
	{
		Text::SP spText( new Text( a_Payload.m_Data, 1.0, false, true ) );
		Log::Status("Conversation", "----- Received Conversation: %s, TextId: %p",
			spText->GetText().c_str(), spText.get() );
		SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spText);
	}
}

void TextExtractor::OnAddAudio(ISensor * a_pSensor)
{
	if ( m_pAudioSensor == NULL )
	{
		m_pAudioSensor= a_pSensor;
		m_pAudioSensor->Subscribe( DELEGATE( TextExtractor, OnAudioData, IData *, this) );
	}
	else
		Log::Warning( "TextExtractor", "multiple audio streams currently not supported by TextExtractor." );
}

void TextExtractor::OnRemoveAudio(ISensor * a_pSensor)
{
	if ( a_pSensor == m_pAudioSensor )
	{
		m_pAudioSensor->Unsubscribe( this );
		m_pAudioSensor = NULL;
	}
}

void TextExtractor::OnAddText(ISensor * a_pSensor)
{
	a_pSensor->Subscribe( DELEGATE( TextExtractor, OnTextData, IData *, this ) );
}

void TextExtractor::OnRemoveText(ISensor * a_pSensor)
{
	a_pSensor->Unsubscribe( this );
}

void TextExtractor::OnAudioData(IData * data)
{
	ISpeechToText * pSTT = SelfInstance::GetInstance()->FindService<ISpeechToText>();
	if ( pSTT != NULL )
	{
		if (!pSTT->IsListening() )
		{
			if ( pSTT->StartListening( DELEGATE(TextExtractor,OnRecognizeSpeech, RecognizeResults *, this ) ) )
				Log::Status("TextExtractor", "TextExtractor started");
			else
				Log::Error("TextExtractor", "Failed to begin listening.");
		}

		float energyAverageSum = 0;
		float energyStandardDeviation = 0;

		AudioData * pAudio = DynamicCast<AudioData>(data);
		if ( pAudio != NULL )
		{
			SpeechAudioData speech;
			speech.m_PCM = pAudio->GetWaveData();		// copy that may want to be a swap()
			speech.m_Bits = pAudio->GetBPS();
			speech.m_Channels = pAudio->GetChannels();
			speech.m_Rate = pAudio->GetFrequency();

			// calculate a energy level from the data..
			if ( speech.m_Bits == 16 )
			{
				short * pSamples = (short *)speech.m_PCM.c_str();
				short maxSample = 0;
				long movingAverage = 0;

				size_t samples = speech.m_PCM.size() / sizeof(short);
				for(size_t i=0;i<samples;++i)
					if ( pSamples[i] > maxSample )
						movingAverage += pSamples[i];

				if ( m_nSpectrumSubs > 0 )
				{
					std::vector<float> fsamples;
					fsamples.resize( samples );
					for(size_t i=0;i<samples;++i)
						fsamples[i] = ((float)pSamples[i]) / 32768.0f;

					if ( m_FFT.GetAverageSize() == 0 )
					{
						m_FFT.Initialize( 1024, (float)speech.m_Rate );
						m_FFT.LogAverages(60, 3);
					}

					m_FFT.Forward(fsamples);
					PublishSpectrum();
				}

				speech.m_Level = movingAverage / (samples * 32768.0f);

				//End of sample period, place into energy average queue
				if(m_EnergyLevelAverages.size() == m_EnergyAverageSampleCount)
					m_EnergyLevelAverages.pop_front();

				m_EnergyLevelAverages.push_back(speech.m_Level);

				//Iterate through all averages in sample deque and calculate overall background energy
				for(std::deque<float>::const_iterator it = m_EnergyLevelAverages.begin(); it != m_EnergyLevelAverages.end(); ++it)
					energyAverageSum += (*it);

				energyAverageSum /= m_EnergyLevelAverages.size();

				//We have the average energy moving average, now we need to determine if the CURRENT energy
				//level is sufficiently higher than this average. To start, let's get the standard deviation
				//and see where the current level falls
				for(std::deque<float>::const_iterator it = m_EnergyLevelAverages.begin(); it != m_EnergyLevelAverages.end(); ++it)
					energyStandardDeviation += (*it - energyAverageSum) * (*it - energyAverageSum);

				energyStandardDeviation = sqrt(energyStandardDeviation / m_EnergyLevelAverages.size());
				//Log::Status( "TextExtractor", "energyStandardDeviation = %f", energyStandardDeviation );

				//Check to see if this speech level is going to change the min/max energy level averages
				if(energyAverageSum < m_MinEnergyAvg)
					m_MinEnergyAvg = energyAverageSum;
				if(energyAverageSum > m_MaxEnergyAvg)
					m_MaxEnergyAvg = energyAverageSum;

				//Normalized value for range calculation
				m_NormalizedEnergyLevel = (energyAverageSum - m_MinEnergyAvg) / (m_MaxEnergyAvg - m_MinEnergyAvg);
				//Find the current confidence threshold
				m_ConfidenceThreshold = m_MaxConfidence - ((m_MaxConfidence - m_MinConfidence) * m_NormalizedEnergyLevel);
        
				if ( std::isnan(m_ConfidenceThreshold) )
					m_ConfidenceThreshold = DEFAULT_CONFIDENCE_THRESHOLD;
			}
			else
			{
				// not supported, so just set the max level..
				Log::Warning("TextExtractor", "Unsupported BPS %u", speech.m_Bits );
				speech.m_Level = 1.0f;
			}

			//If our current energy window has expired, reset the energy threshold timestamp to 0
			if(Time().GetEpochTime() > m_EnergyLevelCrossedTimestamp + m_EnergyTimeInterval)
				m_EnergyLevelCrossedTimestamp = 0.0;
			//If our current speech level is far enough above the standard dev for the background sound energy
			//Set the timestamp to the current time to give us a new window to work with
			if(speech.m_Level > (energyAverageSum + (m_StdDevThreshold * energyStandardDeviation)))
				m_EnergyLevelCrossedTimestamp = Time().GetEpochTime();

			// Apply any filters
			for (size_t i = 0; i < m_Filters.size(); i++)
			{
				const IAudioFilter::SP & spFilter = m_Filters[i];
				if ( spFilter && spFilter->IsActive() )
					spFilter->ApplyFilter(speech);
			}

			// Send to STT
			if ( pSTT->IsConnected())
				pSTT->OnListen(speech);
		}
	}
}

//----------------------------------------------------------------------------

TextExtractor::TranslateReq::TranslateReq( ILanguageTranslation * a_pTranslation, const IThing::SP & a_spFocus, const Text::SP & a_spText, const std::string & a_TargetLang ) :
	m_pTranslation( a_pTranslation ), m_spFocus( a_spFocus ), m_spText(a_spText)
{
	m_pTranslation->Translation(m_spText->GetLanguage(), a_TargetLang, m_spText->GetText(),
			DELEGATE(TranslateReq, OnTranslate, Translations *, this));
}

void TextExtractor::TranslateReq::OnTranslate(Translations * a_pTranslation)
{
	if (a_pTranslation != NULL)
	{
		if (a_pTranslation->m_Translations.size() > 0)
		{
			Translation translation = a_pTranslation->m_Translations[0];
			Log::Status("Conversation", "----- Translated Text: %s -> %s, TextId: %p",
				m_spText->GetText().c_str(), translation.m_Translation.c_str(), m_spText.get());

			m_spText->SetText(translation.m_Translation);
			SelfInstance::GetInstance()->GetTopics()->Publish( "conversation", m_spText->GetText() );
		}

		delete a_pTranslation;
	}
	else
	{
		Log::Error("TextExtractor", "Received Error from LanguageTranslator!");
	}

	m_spFocus->AddChild( m_spText );
	delete this;
}

void TextExtractor::OnSpectrumSubscribe(const ITopics::SubInfo & a_Info )
{
	if ( a_Info.m_Subscribed )
		m_nSpectrumSubs += 1;
	else
		m_nSpectrumSubs -= 1;
}

template<typename T>
inline T MAX( T a, T b )
{
	return a > b ? a : b;
}

void TextExtractor::PublishSpectrum()
{
	int ppb = m_FFTWidth / m_FFT.GetAverageSize();
	int width = m_FFT.GetAverageSize() * ppb;
	int height = m_FFTHeight;

	float fMax = 50.0f;
	if ( m_bAverageMax )
	{
		fMax = 10.0f;
		for(size_t i=0;i<m_FFT.GetAverageSize();++i)
			fMax = MAX( fMax, m_FFT.GetAverage(i) );
	}

	int length = (width * 3) * height;
	unsigned char * RGB = new unsigned char[ length ];
	memset( RGB, 0, length );

	for(int i=0;i< (int)m_FFT.GetAverageSize(); ++i)
	{
		float fBand = m_FFT.GetAverage(i) / fMax;
		if ( fBand > 1.0f )
			fBand = 1.0f;

		int left = i * ppb;
		int right = left + ppb;
		int top = (height - 1) - (int)((height - 1) * fBand);
		int bottom = height;

		if ( top == bottom )
			continue;			// 0, so just skip this band..

		for(int x=left;x<right;++x)
		{
			for(int y=top;y<bottom;++y)
			{
				int offset = (x * 3) + (y * (width * 3));
				RGB[ offset ] = (unsigned char)(0xff * fBand);		// red
				RGB[ offset + 1 ] = 0xff - RGB[offset];				// green
				RGB[ offset + 2 ] = 0x0;							// blue
			}
		}
	}

	std::string jpeg;
	if ( JpegHelpers::EncodeImage( RGB, width, height, 3, jpeg ) )
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		if ( pInstance != NULL )
			pInstance->GetTopics()->Publish( "fft", jpeg, false, true );
	}

	delete [] RGB;
}


