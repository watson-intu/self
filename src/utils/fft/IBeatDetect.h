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


#ifndef IBEAT_DETECT_H
#define IBEAT_DETECT_H

#include "utils/Time.h"
#include "utils/fft/FFT.h"
#include "topics/ITopics.h"

#include <vector>

class IBeatDetect
{
public:
	IBeatDetect() : m_fSampleRate( 0 ), m_nTimeSize( 0 ), m_Sensitivity( 100.0f / 1000.0f )
	{}
	virtual ~IBeatDetect()
	{
		Reset();
	}

	float GetSampleRate() const
	{
		return m_fSampleRate;
	}
	int GetTimeSize() const
	{
		return m_nTimeSize;
	}
	double GetSensitivity() const
	{
		return m_Sensitivity;
	}
	void SetSensitivity( double a_Time )
	{
		m_Sensitivity = a_Time;
	}

	//! Initialize this beat detector
	virtual void Initialize( int a_nTimeSize, float a_SampleRate )
	{
		m_nTimeSize = a_nTimeSize;
		m_fSampleRate = a_SampleRate;
	}

	//! Release this beat detector
	virtual void Reset()
	{}

	//! Returns true if a onset is detected in this block of audio data
	virtual bool Detect( std::vector<float> & a_Samples, size_t a_nOffset ) = 0;

protected:
	//! Data
	float				m_fSampleRate;
	int					m_nTimeSize;
	double				m_Sensitivity;

	static float MAX( float a, float b )
	{
		return a > b ? a : b;
	}

	static float GetAverage(std::vector<float> & arr)
	{
		float avg = 0;
		for (int i = 0; i < (int)arr.size(); i++)
		{
			avg += arr[i];
		}
		avg /= arr.size();
		return avg;
	}

	static float GetSpecAverage(std::vector<float> & arr)
	{
		float avg = 0;
		float num = 0;
		for (int i = 0; i < (int)arr.size(); i++)
		{
			if (arr[i] > 0)
			{
				avg += arr[i];
				num++;
			}
		}
		if (num > 0)
		{
			avg /= num;
		}
		return avg;
	}

	static float GetVariance(std::vector<float> & arr, float val)
	{
		float V = 0;
		for (int i = 0; i < (int)arr.size(); i++)
		{
			V += (float)pow(arr[i] - val, 2);
		}
		V /= arr.size();
		return V;
	}
};

#endif
