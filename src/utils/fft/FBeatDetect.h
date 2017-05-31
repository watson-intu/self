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

class FBeatDetect : public IBeatDetect
{
public:
	FBeatDetect() : m_nSubscriberCount( 0 )
	{}

	virtual void Initialize(int timeSize, float sampleRate);
	virtual void Reset();

	// Analyze the samples in <code>buffer</code>. This is a cumulative
	// process, so you must call this function every frame.
	bool Detect( std::vector<float> & a_Samples, size_t a_nOffset );

	bool IsOnset(int i) const
	{
		return m_bIsOnset[i];
	}

	bool IsKick() const
	{
		int upper = 3 >= m_FFT.GetAverageSize() ? m_FFT.GetAverageSize() : 3;
		return IsRange(0, upper, 2);
	}

	bool IsSnare() const
	{
		int lower = 8 >= m_FFT.GetAverageSize() ? m_FFT.GetAverageSize() : 8;
		int upper = m_FFT.GetAverageSize() - 1;
		int thresh = (upper - lower) / 3 + 1;
		return IsRange(lower, upper, thresh);
	}

	bool IsHat() const
	{
		int lower = m_FFT.GetAverageSize() - 7 < 0 ? 0 : m_FFT.GetAverageSize() - 7;
		int upper = m_FFT.GetAverageSize() - 1;
		return IsRange(lower, upper, 1);
	}

	bool IsRange(int low, int high, int threshold) const
	{
		int num = 0;
		for (int i = low; i < high + 1; i++)
		{
			if (IsOnset(i))
			{
				num++;
			}
		}

		return num >= threshold;
	}

private:
	//! Data
	int					m_nInsertAt;
	std::vector<bool>	m_bIsOnset;
	FFT					m_FFT;
	std::vector<std::vector<float> >
						m_feBuffer;
	std::vector<std::vector<float> >
						m_fdBuffer;
	std::vector<double>	m_Times;
	int					m_nSubscriberCount;

	void InitializeTables();
	void OnSubscriber( const ITopics::SubInfo & a_Info );
	void PublishSpectrum();
};

#endif
