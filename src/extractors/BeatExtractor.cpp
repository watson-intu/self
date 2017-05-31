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


#define DETECT_USING_FREQ			0
#define DETECT_USING_FLUX			1

#include "BeatExtractor.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "sensors/AudioData.h"

#if DETECT_USING_FREQ
#include "utils/fft/FBeatDetect.h"
#elif DETECT_USING_FLUX
#include "utils/fft/F2BeatDetect.h"
#else
#include "utils/fft/EBeatDetect.h"
#endif

const int TIME_SLICE = 1024;

RTTI_IMPL( BeatExtractor, IExtractor );
REG_SERIALIZABLE( BeatExtractor );

BeatExtractor::BeatExtractor() : 
	m_pBT( NULL ),
	m_fBeatHistoryTime( 7.0f ),
	m_nMinBeatCount( 10 ),
	m_fIntervalThreshold( 0.05f ),			// beat intervals need to be within 50 ms of each other
	m_pAudioSensor( NULL ),
	m_bMusicDetected( false ),
	m_fMusicBPM( 0 )
{}

void BeatExtractor::Serialize(Json::Value & json)
{
	IExtractor::Serialize( json );

	json["m_fBeatHistoryTime"] = m_fBeatHistoryTime;
	json["m_nMinBeatCount"] = m_nMinBeatCount;
	json["m_fIntervalThreshold"] = m_fIntervalThreshold;
}

void BeatExtractor::Deserialize(const Json::Value & json)
{
	IExtractor::Deserialize( json );

	if ( json["m_fBeatHistoryTime"].isDouble() )
		m_fBeatHistoryTime = json["m_fBeatHistoryTime"].asFloat();
	if ( json["m_nMinBeatCount"].isInt() )
		m_nMinBeatCount = json["m_nMinBeatCount"].asInt();
	if ( json["m_fIntervalThreshold"].isDouble() )
		m_fIntervalThreshold = json["m_fIntervalThreshold"].asFloat();
}

const char * BeatExtractor::GetName() const
{
	return "BeatExtractor";
}

bool BeatExtractor::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance != NULL )
	{
		pInstance->GetSensorManager()->RegisterForSensor( "AudioData", 
			DELEGATE( BeatExtractor, OnAddAudio, ISensor *, this ),
			DELEGATE( BeatExtractor, OnRemoveAudio, ISensor *, this ) );
	}

	return true;
}

bool BeatExtractor::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance != NULL )
	{
		pInstance->GetSensorManager()->UnregisterForSensor( "AudioData", this );
	}

	return true;
}

void BeatExtractor::OnAddAudio(ISensor * a_pSensor)
{
	if ( m_pAudioSensor == NULL )
	{
		m_pAudioSensor= a_pSensor;
		m_pAudioSensor->Subscribe( DELEGATE( BeatExtractor, OnAudioData, IData *, this) );
	}
	else
		Log::Warning( "TextExtractor", "multiple audio streams currently not supported by TextExtractor." );
}

void BeatExtractor::OnRemoveAudio(ISensor * a_pSensor)
{
	if ( a_pSensor == m_pAudioSensor )
	{
		m_pAudioSensor->Unsubscribe( this );
		m_pAudioSensor = NULL;
	}
}
void BeatExtractor::OnAudioData( IData * a_pData )
{
	AudioData * pAudio = DynamicCast<AudioData>( a_pData );
	if ( pAudio != NULL && pAudio->GetBPS() == 16 )
	{
		if ( m_pBT == NULL || m_pBT->GetSampleRate() != pAudio->GetFrequency() )
		{
			if ( m_pBT == NULL )
			{
#if DETECT_USING_FREQ
				m_pBT = new FBeatDetect();
#elif DETECT_USING_FLUX
				m_pBT = new F2BeatDetect();
#else
				m_pBT = new EBeatDetect();
#endif
				m_pBT->SetSensitivity( 100.0f / 1000.0f );		// 100 ms between beats
			}

			m_pBT->Initialize( TIME_SLICE, (float)pAudio->GetFrequency() );
		}

		size_t nOffset = m_Samples.size();
		std::vector<float> samples( m_Samples );

		short * pSamples = (short *)pAudio->GetWaveData().data();
		size_t nSampleCount = pAudio->GetWaveData().size() / sizeof(short);

		samples.resize( samples.size() + nSampleCount );
		for(size_t i=0;i<nSampleCount;++i)
			samples[i + nOffset] = ((float)pSamples[i]) / 32768.0f;

		// how much time does this audio data represent, subtract this from the current time to know
		double audioDuration = ((double)samples.size() / pAudio->GetChannels()) / (double)pAudio->GetFrequency();
		double audioStartTime = Time().GetEpochTime() - audioDuration;			// get the actual start time for this audio block..

		size_t nProcessed = 0;
		while( (nProcessed + TIME_SLICE) <= samples.size() )
		{
			if ( m_pBT->Detect( samples, nProcessed ) )
			{
				double beatTime = audioStartTime + (((double)nProcessed / (double)pAudio->GetChannels()) / (double)pAudio->GetFrequency());
				m_BeatTimes.push_back( beatTime );
			}

			nProcessed += TIME_SLICE;
		}

		// save any unprocessed samples into a buffer to do later..
		m_Samples.assign( samples.begin() + nProcessed, samples.end() );

		bool musicDetected = false;
		float BPM = 0.0f;
		if ( m_BeatTimes.begin() != m_BeatTimes.end() )
		{
			// clear out any beat history older than a certain amount of time
			double purgeTime = audioStartTime - m_fBeatHistoryTime;
			while( m_BeatTimes.begin() != m_BeatTimes.end() && m_BeatTimes.front() < purgeTime )
				m_BeatTimes.pop_front();

			std::list<double>::iterator iBeat = m_BeatTimes.begin();
			if ( iBeat != m_BeatTimes.end() )
			{
				std::vector<double> intervals;

				double prevBeatTime = *iBeat++;
				double avgInterval = 0.0;
				while( iBeat != m_BeatTimes.end() )
				{
					double interval = *iBeat - prevBeatTime;
					prevBeatTime = *iBeat++;

					intervals.push_back(interval);
					avgInterval += interval;
				}

				if ( intervals.size() >= (size_t)m_nMinBeatCount )
				{
					avgInterval /= intervals.size();	// calculate the average interval between beats

					// validate the intervals are close to the same value, this means we are actualyl detecting music
					bool bIntervalsValid = true;
					for(size_t i=0;i<intervals.size() && bIntervalsValid;++i)
					{
						float diff = (float)fabs( intervals[i] - avgInterval );
						if ( diff > m_fIntervalThreshold )
							bIntervalsValid = false;
					}

					if ( bIntervalsValid )
					{
						musicDetected = true;
						BPM = (float)(60.0f / avgInterval);			// calculate the number of beats per minute
					}
				}
			}
		}

		if ( m_bMusicDetected && !musicDetected )
		{
			Log::Status( "BeatExtractor", "Music stopped." );

			SelfInstance::GetInstance()->GetBlackBoard()->AddThing( 
				IThing::SP( new IThing( TT_PERCEPTION, "MusicStopped", Json::Value::nullRef, 60.0f ) ) );

			m_spBeatTimer.reset();				// stop our beat timer
			m_bMusicDetected = false;
			m_fMusicBPM = 0;
		}
		else if ( !m_bMusicDetected && musicDetected )
		{
			Log::Status( "BeatExtractor", "Music detected, BPM = %.1f", BPM );

			m_bMusicDetected = musicDetected;
			m_fMusicBPM = BPM;

			SelfInstance::GetInstance()->GetBlackBoard()->AddThing( 
				IThing::SP( new IThing( TT_PERCEPTION, "MusicStarted", IThing::JsonObject("BPM", m_fMusicBPM), 60.0f ) ) );

			m_spBeatTimer = TimerPool::Instance()->StartTimer( 
				VOID_DELEGATE( BeatExtractor, OnBeat, this ), 60.0f / (float)m_fMusicBPM, true, true );
		}
		else if ( m_bMusicDetected  )
		{
			m_fMusicBPM = BPM;
			m_spBeatTimer->m_Interval = 60.0f / m_fMusicBPM;
		}
	}
}

void BeatExtractor::OnBeat()
{
	Json::Value data;
	data["BPM"] = m_fMusicBPM;

	SelfInstance::GetInstance()->GetBlackBoard()->AddThing( 
		IThing::SP( new IThing( TT_PERCEPTION, "MusicBeat", data, 60.0f ) ) );
}

