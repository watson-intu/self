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


#ifndef EBEAT_DETECT_H
#define EBEAT_DETECT_H

#include "utils/Time.h"
#include "utils/fft/FFT.h"
#include "IBeatDetect.h"

#include <vector>

class EBeatDetect : public IBeatDetect
{
public:
	EBeatDetect()
	{}

	virtual void Initialize(int timeSize, float sampleRate)
	{
		IBeatDetect::Initialize( timeSize, sampleRate );
		InitializeTables();
	}

	virtual void Reset()
	{
		IBeatDetect::Reset();

		m_eBuffer.clear();
		m_dBuffer.clear();
	}

	// Analyze the samples in <code>buffer</code>. This is a cumulative
	// process, so you must call this function every frame.
	virtual bool Detect( std::vector<float> & a_Samples, size_t a_nOffset )
	{
		if ( a_nOffset >= (int)a_Samples.size() )
			throw WatsonException( "Buffer overflow" );
		if ( (((int)a_Samples.size()) - a_nOffset) < m_nTimeSize )
			throw WatsonException( "Buffer underflow" );

		// compute the energy level
		float level = 0;
		for (int i = 0; i < m_nTimeSize; i++)
			level += (a_Samples[i + a_nOffset] * a_Samples[i + a_nOffset]);

		level /= m_nTimeSize;
		level = (float) sqrt(level);
		float instant = level * 100;
		// compute the average local energy
		float E = GetAverage(m_eBuffer);
		// compute the variance of the energies in eBuffer
		float V = GetVariance(m_eBuffer, E);
		// compute C using a linear digression of C with V
		float C = (-0.0025714f * V) + 1.5142857f;
		// filter negaive values
		float diff = (float)MAX(instant - C * E, 0);
		// find the average of only the positive values in dBuffer
		float dAvg = GetSpecAverage(m_dBuffer);
		// filter negative values
		float diff2 = (float)MAX(diff - dAvg, 0);

		// report false if it's been less than 'sensitivity'
		// milliseconds since the last true value
		double now = Time().GetEpochTime();
		if ( (now - m_Timer) < m_Sensitivity)
		{
			m_bIsOnset = false;
		}
		// if we've made it this far then we're allowed to set a new
		// value, so set it true if it deserves to be, restart the timer
		else if (diff2 > 0 && instant > 2)
		{
			m_bIsOnset = true;
			m_Timer = now;
		}
		// OMG it wasn't true!
		else
		{
			m_bIsOnset = false;
		}
		m_eBuffer[m_nInsertAt] = instant;
		m_dBuffer[m_nInsertAt] = diff;

		if (++m_nInsertAt == m_eBuffer.size())
			m_nInsertAt = 0;

		return m_bIsOnset;
	}

private:
	//! Data
	bool			m_bIsOnset;
	std::vector<float>
					m_eBuffer;
	std::vector<float>
					m_dBuffer;

	int				m_nInsertAt;
	double			m_Timer;

	void InitializeTables()
	{
		m_bIsOnset = false;
		m_eBuffer.resize( (size_t)( m_fSampleRate / m_nTimeSize ) );
		m_dBuffer.resize( (size_t)( m_fSampleRate / m_nTimeSize ) );
		m_Timer = Time().GetEpochTime();
		m_nInsertAt = 0;
	}
};

#endif
