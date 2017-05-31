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


#ifndef FFT_H
#define FFT_H

#include "IFourierTransform.h"

class FFT : public IFourierTransform
{
public:
	void Initialize(int a_nTimeSize, float a_nSampleRate, IWindowFunction * a_pFunction = NULL )
	{
		IFourierTransform::Initialize(a_nTimeSize, a_nSampleRate, a_pFunction );

		if ((a_nTimeSize & (a_nTimeSize - 1)) != 0)
			throw WatsonException("FFT: timeSize must be a power of two.");

		BuildReverseTable();
		BuildTrigTables();
	}

	//! FourierTransform interface
	virtual void AllocateArrays()
	{
		m_Spectrum.resize(m_nTimeSize / 2 + 1);
		m_Real.resize(m_nTimeSize);
		m_Imag.resize(m_nTimeSize);
	}

	virtual void ScaleBand(int i, float s)
	{
		if (s < 0)
		{
			Log::Error("FFT", "Can't scale a frequency band by a negative value.");
			return;
		}

		m_Real[i] *= s;
		m_Imag[i] *= s;
		m_Spectrum[i] *= s;

		if (i != 0 && i != m_nTimeSize / 2)
		{
			m_Real[m_nTimeSize - i] = m_Real[i];
			m_Imag[m_nTimeSize - i] = -m_Imag[i];
		}
	}

	virtual void SetBand(int i, float a)
	{
		if (a < 0)
		{
			Log::Error("FFT", "Can't set a frequency band to a negative value.");
			return;
		}

		if (m_Real[i] == 0 && m_Imag[i] == 0)
		{
			m_Real[i] = a;
			m_Spectrum[i] = a;
		}
		else
		{
			m_Real[i] /= m_Spectrum[i];
			m_Imag[i] /= m_Spectrum[i];
			m_Spectrum[i] = a;
			m_Real[i] *= m_Spectrum[i];
			m_Imag[i] *= m_Spectrum[i];
		}
		if (i != 0 && i != m_nTimeSize / 2)
		{
			m_Real[m_nTimeSize - i] = m_Real[i];
			m_Imag[m_nTimeSize - i] = -m_Imag[i];
		}
	}

	virtual void Forward(std::vector<float> & a_Samples, int a_nOffset = 0 )
	{
		if ( a_nOffset >= (int)a_Samples.size() )
			throw WatsonException( "Buffer overflow" );
		if ( ((int)a_Samples.size() - a_nOffset) < m_nTimeSize )
			throw WatsonException( "Buffer underflow" );

		DoWindow(a_Samples, a_nOffset);
		// copy samples to real/imag in bit-reversed order
		BitReverseSamples(a_Samples, a_nOffset);
		// perform the fft
		DoFFT();
		// fill the spectrum buffer with amplitudes
		FillSpectrum();
	}

	virtual void Inverse(std::vector<float> & a_Samples, int a_nOffset = 0 )
	{
		if ( a_nOffset >= (int)a_Samples.size() )
			throw WatsonException( "Buffer overflow" );
		if ( ((int)a_Samples.size() - a_nOffset) < m_nTimeSize )
			throw WatsonException( "Buffer underflow" );

		// conjugate
		for (int i = 0; i < m_nTimeSize; i++)
			m_Imag[i] *= -1;

		BitReverseComplex();
		DoFFT();

		// copy the result in real into buffer, scaling as we do
		for (int i = 0; i < (int)m_Real.size(); i++)
			a_Samples[i + a_nOffset] = m_Real[i] / m_Real.size();
	}


private:
	//! Data
	std::vector<int> m_Reverse;
	std::vector<float> m_SIN;
	std::vector<float> m_COS;

	float SIN(int i) const
	{
		return m_SIN[i];
	}

	float COS(int i) const
	{
		return m_COS[i];
	}

	void BuildReverseTable()
	{
		int N = m_nTimeSize;
		m_Reverse.resize(N);

		// set up the bit reversing table
		m_Reverse[0] = 0;
		for (int limit = 1, bit = N / 2; limit < N; limit <<= 1, bit >>= 1)
			for (int i = 0; i < limit; i++)
				m_Reverse[i + limit] = m_Reverse[i] + bit;
	}

	void BuildTrigTables()
	{
		int N = m_nTimeSize;
		m_SIN.resize(N);
		m_COS.resize(N);

		for (int i = 0; i < N; i++)
		{
			m_SIN[i] = (float) ::sin(-(float)PI / i);
			m_COS[i] = (float) ::cos(-(float)PI / i);
		}
	}

	// copies the values in the samples array into the real array
	// in bit reversed order. the imag array is filled with zeros.
	void BitReverseSamples(std::vector<float> & samples, int startAt)
	{
		for (int i = 0; i < m_nTimeSize; ++i)
		{
			m_Real[i] = samples[startAt + m_Reverse[i]];
			m_Imag[i] = 0.0f;
		}
	}

	// bit reverse real[] and imag[]
	void BitReverseComplex()
	{
		std::vector<float> revReal;
		revReal.resize(m_Real.size());
		std::vector<float> revImag;
		revImag.resize(m_Imag.size());

		for (int i = 0; i < (int)m_Real.size(); i++)
		{
			revReal[i] = m_Real[m_Reverse[i]];
			revImag[i] = m_Imag[m_Reverse[i]];
		}
		m_Real = revReal;
		m_Imag = revImag;
	}

	// performs an in-place fft on the data in the real and imag arrays
	// bit reversing is not necessary as the data will already be bit reversed
	void DoFFT()
	{
		for (int halfSize = 1; halfSize < (int)m_Real.size(); halfSize *= 2)
		{
			// float k = -(float)Math.PI/halfSize;
			// phase shift step
			// float phaseShiftStepR = (float)Math.cos(k);
			// float phaseShiftStepI = (float)Math.sin(k);
			// using lookup table
			float phaseShiftStepR = COS(halfSize);
			float phaseShiftStepI = SIN(halfSize);
			// current phase shift
			float currentPhaseShiftR = 1.0f;
			float currentPhaseShiftI = 0.0f;
			for (int fftStep = 0; fftStep < halfSize; fftStep++)
			{
				for (int i = fftStep; i < (int)m_Real.size(); i += 2 * halfSize)
				{
					int off = i + halfSize;
					float tr = (currentPhaseShiftR * m_Real[off]) - (currentPhaseShiftI * m_Imag[off]);
					float ti = (currentPhaseShiftR * m_Imag[off]) + (currentPhaseShiftI * m_Real[off]);
					m_Real[off] = m_Real[i] - tr;
					m_Imag[off] = m_Imag[i] - ti;
					m_Real[i] += tr;
					m_Imag[i] += ti;
				}
				float tmpR = currentPhaseShiftR;
				currentPhaseShiftR = (tmpR * phaseShiftStepR) - (currentPhaseShiftI * phaseShiftStepI);
				currentPhaseShiftI = (tmpR * phaseShiftStepI) + (currentPhaseShiftI * phaseShiftStepR);
			}
		}
	}

};

#endif
