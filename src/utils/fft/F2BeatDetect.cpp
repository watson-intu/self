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


#include "topics/TopicManager.h"
#include "utils/JpegHelpers.h"

#include "F2BeatDetect.h"
#include "SelfInstance.h"

void F2BeatDetect::Initialize(int timeSize, float sampleRate)
{
	IBeatDetect::Initialize(timeSize, sampleRate);
	InitializeTables();

	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance != NULL)
	{
		pInstance->GetTopics()->RegisterTopic("f2beatdetect", "image/jpeg",
			DELEGATE(F2BeatDetect, OnSubscriber, const ITopics::SubInfo &, this));
	}
}

void F2BeatDetect::Reset()
{
	IBeatDetect::Reset();

	m_LastSpec.clear();
	m_Flux.clear();
	m_Threshold.clear();

	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance != NULL)
		pInstance->GetTopics()->UnregisterTopic("f2beatdetect");
}

// Analyze the samples in <code>buffer</code>. This is a cumulative
// process, so you must call this function every frame.
bool F2BeatDetect::Detect(std::vector<float> & a_Samples, size_t a_nOffset)
{
	if (a_nOffset >= (int)a_Samples.size())
		throw WatsonException("Buffer overflow");
	if (((int)(a_Samples.size() - a_nOffset)) < m_nTimeSize)
		throw WatsonException("Buffer underflow");

	m_FFT.Forward(a_Samples, a_nOffset);

	// calculate the flux of the lower frequencies only..
	float flux = 0.0f;
	int lower = (int)(m_FFT.GetSpecSize() * 0.6f);
	int upper = (int)(m_FFT.GetSpecSize() * 0.9f);
	for (int i = lower; i < upper; ++i)
	{
		float fValue = m_FFT.GetBand(i);
		float fDelta = fValue - m_LastSpec[i];
		m_LastSpec[i] = fValue;

		if (fDelta > 0.0f)
			flux += fDelta;
	}

	m_Flux[m_nInsertAt] = flux;
	float fThreshold = GetAverage(m_Flux) * m_ThresholdScale;
	m_Threshold[m_nInsertAt] = fThreshold;

	bool bBeat = false;
	if (flux > fThreshold)
	{
		if (!m_bOnset)
		{
			m_bOnset = true;
			bBeat = true;
		}
	}
	else if (m_bOnset)
	{
		// clear the flag again
		m_bOnset = false;
	}

	if (m_nSubscriberCount > 0)
		PublishSpectrum();

	if (++m_nInsertAt == m_Flux.size())
		m_nInsertAt = 0;

	return bBeat;
}

void F2BeatDetect::InitializeTables()
{
	m_FFT.Initialize(m_nTimeSize, (float)m_fSampleRate);
	m_FFT.SetWindow(new HammingWindow());
	//m_FFT.LogAverages(60, 3);
	//m_FFT.LinearAverages(60);

	m_LastSpec.resize(m_FFT.GetSpecSize());
	m_Flux.resize(m_WindowSize);
	m_Threshold.resize(m_WindowSize);
	m_nInsertAt = 0;
}

void F2BeatDetect::OnSubscriber(const ITopics::SubInfo & a_Info)
{
	if (a_Info.m_Subscribed)
		m_nSubscriberCount += 1;
	else
		m_nSubscriberCount -= 1;
}

void F2BeatDetect::PublishSpectrum()
{
	const int PIXELS_PER_BAND = 5;

	int width = m_WindowSize * PIXELS_PER_BAND;
	int height = 100;

	static float fMax = 0.0f;
	for (int i = 0; i < m_WindowSize; ++i)
		fMax = MAX(fMax, m_Flux[i]);

	int length = (width * 3) * height;
	unsigned char * RGB = new unsigned char[length];
	memset(RGB, 0, length);

	for (int i = 0; i < m_WindowSize; ++i)
	{
		float fBand = m_Flux[i] / fMax;
		if (fBand > 1.0f)
			fBand = 1.0f;

		int left = i * PIXELS_PER_BAND;
		int right = left + PIXELS_PER_BAND;
		int top = (height - 1) - (int)((height - 1) * fBand);
		int bottom = height;

		if (top == bottom)
			continue;			// 0, so just skip this band..

		for (int x = left; x < right; ++x)
		{
			if ( i == m_nInsertAt )
			{
				for( int y = 0; y < height; ++y)
					RGB[ (x * 3) + ( y * (width * 3) ) + 2 ]  = 0xff;
			}

			for (int y = top; y < bottom; ++y)
			{
				int offset = (x * 3) + (y * (width * 3));

				if (m_Flux[i] > m_Threshold[i])
					RGB[offset] = 0xff;
				else
					RGB[offset + 1] = 0xff;
			}
		}
	}

	std::string jpeg;
	if (JpegHelpers::EncodeImage(RGB, width, height, 3, jpeg))
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		if (pInstance != NULL)
			pInstance->GetTopics()->Publish("f2beatdetect", jpeg, false, true);
	}

	delete[] RGB;
}

