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


#ifndef SELF_BEAT_EXTRACTOR_H
#define SELF_BEAT_EXTRACTOR_H

#include <math.h>

#include "extractors/TextExtractor.h"
#include "SelfLib.h"

class IBeatDetect;

//! This filter doesn't filter audio, it just detects music beats and posts a Beat
//! object to the blackboard when a music beat is detected.
class SELF_API BeatExtractor : public IExtractor
{
public:
	RTTI_DECL();

	//! Construction
	BeatExtractor();

	//! ISerialziable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IFeatureExtractor interface
	virtual const char * GetName() const;
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Data
	float		m_fBeatHistoryTime;			// how many seconds of beat history to keep
	int			m_nMinBeatCount;			// how many beats do we need to detect music
	float		m_fIntervalThreshold;

	IBeatDetect *
				m_pBT;						// beat detection that uses an FFT to detect the beat
	std::vector<float>
				m_Samples;					// buffer for samples yet to be processed.

	ISensor *	m_pAudioSensor;
	bool		m_bMusicDetected;
	std::list<double>
				m_BeatTimes;				// history of beat times
	float		m_fMusicBPM;				// detected BPM
	TimerPool::ITimer::SP 
				m_spBeatTimer;				// timer to fire on the down-beat...

	void		OnAddAudio(ISensor * a_pSensor);
	void		OnRemoveAudio(ISensor * a_pSensor);
	void		OnAudioData( IData * a_pData );
	void		OnBeat();
};

#endif // SELF_BEAT_FILTER_H

