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


#ifndef DFT_H
#define DFT_H

#include "IFourierTransform.h"

class DFT : public IFourierTransform
{
public:
	/**
	* Constructs a DFT that expects audio buffers of length <code>timeSize</code> that
	* have been recorded with a sample rate of <code>sampleRate</code>. Will throw an
	* IllegalArgumentException if <code>timeSize</code> is not even.
	*
	* @param timeSize the length of the audio buffers you plan to analyze
	* @param sampleRate the sample rate of the audio samples you plan to analyze
	*/
	void Initialize(int timeSize, float sampleRate)
	{
		IFourierTransform::Initialize(timeSize, sampleRate);
		if (timeSize % 2 != 0)
			throw WatsonException("DFT: timeSize must be even.");
		buildTrigTables();
	}

	//! FourierTransform interface
	virtual void AllocateArrays()
	{
		m_Spectrum.resize( m_nTimeSize / 2 + 1 );
		m_Real.resize( m_nTimeSize / 2 + 1 );
		m_Imag.resize( m_nTimeSize / 2 + 1 );
	}

	virtual void ScaleBand(int i, float s)
	{}

	virtual void SetBand(int i, float a)
	{}

	virtual void Forward(std::vector<float> & a_Samples, int a_nOffset = 0)
	{
		if ( a_nOffset >= a_Samples.size() )
			throw WatsonException( "Buffer overflow" );
		if ( (a_Samples.size() - a_nOffset) < m_nTimeSize )
			throw WatsonException( "Buffer underflow" );

		DoWindow(a_Samples, a_nOffset);

		int N = m_nTimeSize;
		for (int f = 0; f <= N / 2; f++)
		{
			m_Real[f] = 0.0f;
			m_Imag[f] = 0.0f;
			for (int t = 0; t < N; t++)
			{
				m_Real[f] += a_Samples[t + a_nOffset] * COS(t * f);
				m_Imag[f] += a_Samples[t + a_nOffset] * -SIN(t * f);
			}
		}
		FillSpectrum();
	}

	virtual void Inverse(std::vector<float> & a_Samples, int a_nOffset = 0 )
	{
		if ( a_nOffset >= a_Samples.size() )
			throw WatsonException( "Buffer overflow" );
		if ( (a_Samples.size() - a_nOffset) < m_nTimeSize )
			throw WatsonException( "Buffer underflow" );

		int N = m_nTimeSize;
		m_Real[0] /= N;
		m_Imag[0] = -m_Imag[0] / (N / 2);
		m_Real[N / 2] /= N;
		m_Imag[N / 2] = -m_Imag[0] / (N / 2);
		for (int i = 0; i < N / 2; i++)
		{
			m_Real[i] /= (N / 2);
			m_Imag[i] = -m_Imag[i] / (N / 2);
		}
		for (int t = 0; t < N; t++)
		{
			a_Samples[t] = 0.0f;
			for (int f = 0; f < N / 2; f++)
			{
				a_Samples[t + a_nOffset] += m_Real[f] * COS(t * f) + m_Imag[f] * SIN(t * f);
			}
		}
	}

private:
	// lookup table data and functions
	std::vector<float> m_SIN;
	std::vector<float> m_COS;

	void buildTrigTables()
	{
		int N = m_Spectrum.size() * m_nTimeSize;
		m_SIN.resize(N);
		m_COS.resize(N);
		for (int i = 0; i < N; i++)
		{
			m_SIN[i] = (float) ::sin(i * TWO_PI / m_nTimeSize);
			m_COS[i] = (float) ::cos(i * TWO_PI / m_nTimeSize);
		}
	}

	float SIN(int i) const
	{
		return m_SIN[i];
	}

	float COS(int i) const
	{
		return m_COS[i];
	}
};

#endif


