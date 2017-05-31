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


#include "FourierFilters.h"

#include <iostream>
#include <fstream>

#ifdef _WIN32
#define M_E 2.71828182845904523536
#define M_LOG2E 1.44269504088896340736
#define M_LOG10E 0.434294481903251827651
#define M_LN2 0.693147180559945309417
#define M_LN10 2.30258509299404568402
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.785398163397448309616
#define M_1_PI 0.318309886183790671538
#define M_2_PI 0.636619772367581343076
#define M_1_SQRTPI 0.564189583547756286948
#define M_2_SQRTPI 1.12837916709551257390
#define M_SQRT2 1.41421356237309504880
#define M_SQRT_2 0.707106781186547524401
#endif

REG_SERIALIZABLE(FourierFilters);
RTTI_IMPL(FourierFilters, IAudioFilter);
RTTI_IMPL_BASE_EMBEDDED(FourierFilters, IFourierFilter);

FourierFilters::FourierFilters() :
	m_LastInputSz(0),
	m_NextPow2(0),
	m_LastBitRate(0)
{}

void FourierFilters::Serialize(Json::Value & json)
{
	SerializeVector("m_Filters", m_Filters, json );
}

void FourierFilters::Deserialize(const Json::Value & json)
{
	DeserializeVector("m_Filters", json, m_Filters);
}

void FourierFilters::ApplyFilter(SpeechAudioData & a_Data)
{
	if (!m_bActive || m_Filters.size() == 0)
		return;

	// Verify that size of input has not changed...
	size_t sz = a_Data.m_PCM.size() / sizeof(short);
	if (m_LastInputSz != sz || m_LastBitRate != a_Data.m_Rate)
	{
		// Reconfigure if needed
		m_LastInputSz = sz;
		m_LastBitRate = a_Data.m_Rate;
		m_NextPow2 = NextPow2(sz);
		Log::Debug("FourierFilters", "New next power of 2: %d...", m_NextPow2);
		Log::Debug("FourierFilters", "size by .length() fx : %d", a_Data.m_PCM.length());
	}

	// Convert from raw string data to array of complex numbers    
	short * samples = (short *)a_Data.m_PCM.c_str();

	// Create complex array and pad zeros if necessary
	ComplexNumArray fft_data(m_NextPow2);
	for (size_t i = 0; i < sz; i++)
		fft_data[i] = ComplexNum(samples[i], 0.0);

	// FFT
	FFT(fft_data);

	// Apply any filters in the frequency domain
	bool bAudioModified = false;
	for (size_t i = 0; i < m_Filters.size(); i++)
	{
		const IFourierFilter::SP & spFilter = m_Filters[i];
		if ( spFilter )
			bAudioModified |= spFilter->ApplyFilter(fft_data);
	}

	// IFFT
	if ( bAudioModified )
	{
		IFFT(fft_data);

		// Back to PCM
		for (size_t i = 0; i < sz; i++)
			samples[i] = (short)fft_data[i].real();
	}
}

void FourierFilters::AddFilter(IFourierFilter::SP a_Filter)
{
	m_Filters.push_back(a_Filter);
}

//! Helpers for Background Noise Filtering
void FourierFilters::FFT(ComplexNumArray & x)
{
	//Cooley-Tukey algorithm recursive function
	const size_t sz = x.size();
	if (sz <= 1)
		return; //		Smallest sub-fft, (X = x)
	// Split into odd and even elements
	ComplexNumArray sub_even = x[std::slice(0, sz / 2, 2)];
	ComplexNumArray sub_odd = x[std::slice(1, sz / 2, 2)];
	// Recursive calls
	FFT(sub_even);
	FFT(sub_odd);
	// Recombine subsets that have already been transformed
	for (size_t k = 0; k < sz / 2; k++)
	{
		ComplexNum odd_coeff = (ComplexNum)std::polar(1.0, -2.0 * M_PI * float(k) / float(sz));
		x[k] = sub_even[k] + odd_coeff * sub_odd[k];
		x[k + sz / 2] = sub_even[k] - odd_coeff * sub_odd[k];
	}
}

void FourierFilters::IFFT(ComplexNumArray & X)
{
	X = X.apply(std::conj);
	FFT(X);
	X = X.apply(std::conj);
	X /= (float)X.size();
}

int FourierFilters::NextPow2(int i)
{
	int temp = i;
	while (temp % 2 == 0)
		temp /= 2;
	if (temp == 1)
		return i;
	else
		return NextPow2(i + 1);
}


void FourierFilters::SaveArrayToCSV(const ComplexNumArray & array, const char * filename)
{
	Log::Debug("NoiseFilter", "Writing... %s", filename);
	std::ofstream csv_file;
	csv_file.open(filename);
	for (size_t i = 0; i < array.size(); i++)
	{
		csv_file << i << ',' << array[i].real() << ',' << array[i].imag() << ',' << '\n';
	}
	csv_file.close();
}
