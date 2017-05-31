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


#ifndef FBEAT_DETECT_H
#define FBEAT_DETECT_H

#include "utils/Time.h"
#include "utils/fft/FFT.h"
#include "utils/fft/HammingWindow.h"
#include "topics/ITopics.h"
#include "IBeatDetect.h"

#include <vector>

class F2BeatDetect : public IBeatDetect
{
public:
	F2BeatDetect() : 
		m_ThresholdScale( 1.5f), 
		m_WindowSize( 20 ), 
		m_bOnset( false ), 
		m_nInsertAt( 0 ), 
		m_nSubscriberCount( 0 )
	{}

	virtual void Initialize(int timeSize, float sampleRate);
	virtual void Reset();

	// Analyze the samples in <code>buffer</code>. This is a cumulative
	// process, so you must call this function every frame.
	bool Detect( std::vector<float> & a_Samples, size_t a_nOffset );

private:
	//! Data
	int					m_WindowSize;
	float				m_ThresholdScale;
	FFT					m_FFT;
	bool				m_bOnset;
	std::vector<float>	m_LastSpec;

	int					m_nInsertAt;
	std::vector<float>	m_Flux;
	std::vector<float>	m_Threshold;
	int					m_nSubscriberCount;

	void InitializeTables();
	void OnSubscriber( const ITopics::SubInfo & a_Info );
	void PublishSpectrum();
};

#endif
