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


#ifndef FOURIER_TRANSFORM_H
#define FOURIER_TRANSFORM_H

#include "utils/Log.h"
#include "utils/fft/RectangularWindow.h"

#include <vector>

class IFourierTransform
{
public:
	//! Constants
	static const float PI;
	static const float TWO_PI;

	//! Construction
	IFourierTransform() : m_nTimeSize(0), m_nSampleRate(0), m_fBandWidth(0), m_pWindow(NULL)
	{}
	virtual ~IFourierTransform()
	{
		if (m_pWindow != NULL)
			delete m_pWindow;
	}

	virtual void AllocateArrays() = 0;
	virtual void SetBand(int i, float a) = 0;
	virtual void ScaleBand(int i, float s) = 0;
	virtual void Forward(std::vector<float> & buffer, int a_nOffset = 0 ) = 0;
	virtual void Inverse(std::vector<float> & buffer, int a_nOffset = 0 ) = 0;

	void Initialize(int a_TimeSize, float a_SampleRate, IWindowFunction * a_pFunction = NULL )
	{
		m_nTimeSize = a_TimeSize;
		m_nSampleRate = (int)a_SampleRate;
		m_fBandWidth = (2.0f / m_nTimeSize) * ((float)m_nSampleRate / 2.0f);
		m_pWindow = a_pFunction;

		NoAverages();
		AllocateArrays();
	}

	int GetTimeSize() const
	{
		return m_nTimeSize;
	}

	size_t GetSpecSize() const
	{
		return m_Spectrum.size();
	}

	float GetBand(int i) const
	{
		return m_Spectrum[i];
	}
	float GetBandWidth() const
	{
		return m_fBandWidth;
	}

	float GetAverageBandWidth(int averageIndex) const
	{
		if (m_nWhichAverage == LINAVG)
		{
			// an average represents a certain number of bands in the spectrum
			int avgWidth = (int)((float)m_Spectrum.size() / (float)m_Averages.size());
			return avgWidth * GetBandWidth();

		}
		else if (m_nWhichAverage == LOGAVG)
		{
			// which "octave" is this index in?
			int octave = averageIndex / m_AvgPerOctave;
			float lowFreq, hiFreq, freqStep;
			// figure out the low frequency for this octave
			if (octave == 0)
			{
				lowFreq = 0;
			}
			else
			{
				lowFreq = (m_nSampleRate / 2) / (float)pow(2, m_nOctaves - octave);
			}
			// and the high frequency for this octave
			hiFreq = (m_nSampleRate / 2) / (float)pow(2, m_nOctaves - octave - 1);
			// each average band within the octave will be this big
			freqStep = (hiFreq - lowFreq) / m_AvgPerOctave;

			return freqStep;
		}

		return 0;
	}

	// Returns the index of the frequency band that contains the requested
	// frequency.
	int FreqToIndex(float freq) const
	{
		// special case: freq is lower than the bandwidth of spectrum[0]
		if (freq < GetBandWidth() / 2)
			return 0;
		// special case: freq is within the bandwidth of spectrum[spectrum.length - 1]
		if (freq > m_nSampleRate / 2 - GetBandWidth() / 2)
			return m_Spectrum.size() - 1;
		// all other cases
		float fraction = freq / (float)m_nSampleRate;
		int i = (int)((m_nTimeSize * fraction) + 0.5f);
		return i;
	}

	// Returns the middle frequency of the i<sup>th</sup> band.
	float IndexToFreq(int i) const
	{
		float bw = GetBandWidth();
		// special case: the width of the first bin is half that of the others.
		//               so the center frequency is a quarter of the way.
		if (i == 0) return bw * 0.25f;
		// special case: the width of the last bin is half that of the others.
		if (i == m_Spectrum.size() - 1)
		{
			float lastBinBeginFreq = (m_nSampleRate / 2) - (bw / 2);
			float binHalfWidth = bw * 0.25f;
			return lastBinBeginFreq + binHalfWidth;
		}
		// the center frequency of the ith band is simply i*bw
		// because the first band is half the width of all others.
		// treating it as if it wasn't offsets us to the middle 
		// of the band.
		return i*bw;
	}

	// Returns the center frequency of the i<sup>th</sup> average band.
	float GetAverageCenterFrequency(int i) const
	{
		if (m_nWhichAverage == LINAVG)
		{
			// an average represents a certain number of bands in the spectrum
			int avgWidth = (int)((float)m_Spectrum.size() / (float)m_Averages.size());
			// the "center" bin of the average, this is fudgy.
			int centerBinIndex = i*avgWidth + avgWidth / 2;
			return IndexToFreq(centerBinIndex);

		}
		else if (m_nWhichAverage == LOGAVG)
		{
			// which "octave" is this index in?
			int octave = i / m_AvgPerOctave;
			// which band within that octave is this?
			int offset = i % m_AvgPerOctave;
			float lowFreq, hiFreq, freqStep;
			// figure out the low frequency for this octave
			if (octave == 0)
			{
				lowFreq = 0;
			}
			else
			{
				lowFreq = (m_nSampleRate / 2) / (float)pow(2, m_nOctaves - octave);
			}
			// and the high frequency for this octave
			hiFreq = (m_nSampleRate / 2) / (float)pow(2, m_nOctaves - octave - 1);
			// each average band within the octave will be this big
			freqStep = (hiFreq - lowFreq) / m_AvgPerOctave;
			// figure out the low frequency of the band we care about
			float f = lowFreq + offset*freqStep;
			// the center of the band will be the low plus half the width
			return f + freqStep / 2;
		}

		return 0;
	}

	// Gets the amplitude of the requested frequency in the spectrum.
	float GetFreq(float freq) const
	{
		return GetBand(FreqToIndex(freq));
	}

	// Sets the amplitude of the requested frequency in the spectrum to a
	void SetFreq(float freq, float a)
	{
		SetBand(FreqToIndex(freq), a);
	}

	// Scales the amplitude of the requested frequency by a
	void ScaleFreq(float freq, float s)
	{
		ScaleBand(FreqToIndex(freq), s);
	}

	// Returns the number of averages currently being calculated.
	size_t GetAverageSize() const
	{
		return m_Averages.size();
	}

	// Gets the value of the <code>i<sup>th</sup></code> average.
	float GetAverage(int i) const
	{
		float ret;
		if (i >= 0 && i < (int)m_Averages.size())
			ret = m_Averages[i];
		else
			ret = 0;
		return ret;
	}

	// Calculate the average amplitude of the frequency band bounded by
	// <code>lowFreq</code> and <code>hiFreq</code>, inclusive.
	float CalculateAverage(float lowFreq, float hiFreq)
	{
		int lowBound = FreqToIndex(lowFreq);
		int hiBound = FreqToIndex(hiFreq);
		float avg = 0;
		for (int i = lowBound; i <= hiBound; i++)
		{
			avg += m_Spectrum[i];
		}
		avg /= (hiBound - lowBound + 1);
		return avg;
	}

	// Get the Real part of the Complex representation of the spectrum.
	const std::vector<float> & GetSpectrumReal() const
	{
		return m_Real;
	}

	// Get the Imaginary part of the Complex representation of the spectrum.
	const std::vector<float> GetSpectrumImaginary() const
	{
		return m_Imag;
	}

	// Performs an inverse transform of the frequency spectrum represented by
	// freqReal and freqImag and places the result in buffer.
	void Inverse(const std::vector<float> & freqReal, const std::vector<float> & freqImag,
		std::vector<float> & buffer)
	{
		SetComplex(freqReal, freqImag);
		Inverse(buffer);
	}

	void NoAverages()
	{
		m_Averages.clear();
		m_nWhichAverage = NOAVG;
	}

	void LinearAverages(int numAvg)
	{
		if (numAvg > (int)m_Spectrum.size() / 2)
		{
			Log::Error("FourierTransform", "The number of averages for this transform can be at most %d.", m_Spectrum.size() / 2);
		}
		else
		{
			m_Averages.resize(numAvg);
			m_nWhichAverage = LINAVG;
		}
	}

	void LogAverages(int minBandwidth, int bandsPerOctave)
	{
		float nyq = (float)m_nSampleRate / 2.0f;
		m_nOctaves = 1;
		while ((nyq /= 2) > minBandwidth)
		{
			m_nOctaves++;
		}
		Log::Debug("FourierTransform", "Number of octaves = %d", m_nOctaves);

		m_AvgPerOctave = bandsPerOctave;
		m_Averages.resize(m_nOctaves * bandsPerOctave);
		m_nWhichAverage = LOGAVG;
	}

	void SetWindow(IWindowFunction * windowFunction)
	{
		if (m_pWindow != NULL)
			delete m_pWindow;

		m_pWindow = windowFunction;
	}


protected:
	//! Constants
	static int LINAVG;
	static int LOGAVG;
	static int NOAVG;

	//! Data
	int m_nTimeSize;
	int m_nSampleRate;
	float m_fBandWidth;
	IWindowFunction * m_pWindow;
	std::vector<float> m_Real;
	std::vector<float> m_Imag;
	std::vector<float> m_Spectrum;
	std::vector<float> m_Averages;
	int m_nWhichAverage;
	int m_nOctaves;
	int m_AvgPerOctave;

	void SetComplex(const std::vector<float> & r, const std::vector<float> & i)
	{
		if (m_Real.size() != r.size() || m_Imag.size() != i.size())
			throw new WatsonException("FourierTransform.setComplex: the two arrays must be the same length as their member counterparts.");

		m_Real = r;
		m_Imag = i;
	}

	// fill the spectrum array with the amps of the data in real and imag
	// used so that this class can handle creating the average array
	// and also do spectrum shaping if necessary
	void FillSpectrum()
	{
		for (size_t i = 0; i < m_Spectrum.size(); i++)
		{
			m_Spectrum[i] = (float)sqrt(m_Real[i] * m_Real[i] + m_Imag[i] * m_Imag[i]);
		}

		if (m_nWhichAverage == LINAVG)
		{
			int avgWidth = (int)m_Spectrum.size() / m_Averages.size();
			for (size_t i = 0; i < m_Averages.size(); i++)
			{
				float avg = 0;
				int j;
				for (j = 0; j < avgWidth; j++)
				{
					int offset = j + i * avgWidth;
					if (offset < (int)m_Spectrum.size())
					{
						avg += m_Spectrum[offset];
					}
					else
					{
						break;
					}
				}
				avg /= j + 1;
				m_Averages[i] = avg;
			}
		}
		else if (m_nWhichAverage == LOGAVG)
		{
			for (int i = 0; i < m_nOctaves; i++)
			{
				float lowFreq, hiFreq, freqStep;
				if (i == 0)
				{
					lowFreq = 0;
				}
				else
				{
					lowFreq = (m_nSampleRate / 2) / (float)pow(2, m_nOctaves - i);
				}
				hiFreq = (m_nSampleRate / 2) / (float)pow(2, m_nOctaves - i - 1);
				freqStep = (hiFreq - lowFreq) / m_AvgPerOctave;
				float f = lowFreq;
				for (int j = 0; j < m_AvgPerOctave; j++)
				{
					int offset = j + i * m_AvgPerOctave;
					m_Averages[offset] = CalculateAverage(f, f + freqStep);
					f += freqStep;
				}
			}
		}
	}

	void DoWindow(std::vector<float> & samples, int a_nOffset = 0 )
	{
		if ( m_pWindow != NULL )
			m_pWindow->Apply(samples, a_nOffset, m_nTimeSize );
	}

};

#endif
