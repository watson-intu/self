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

#include "FBeatDetect.h"
#include "SelfInstance.h"

void FBeatDetect::Initialize(int timeSize, float sampleRate)
{
	IBeatDetect::Initialize( timeSize, sampleRate );
	InitializeTables();

	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance != NULL )
	{
		pInstance->GetTopics()->RegisterTopic( "fbeatdetect", "image/jpeg",
			DELEGATE( FBeatDetect, OnSubscriber, const ITopics::SubInfo &, this ) );
	}
}

void FBeatDetect::Reset()
{
	IBeatDetect::Reset();

	m_bIsOnset.clear();
	m_feBuffer.clear();
	m_fdBuffer.clear();
	m_Times.clear();

	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance != NULL )
		pInstance->GetTopics()->UnregisterTopic( "fbeatdetect" );
}

// Analyze the samples in <code>buffer</code>. This is a cumulative
// process, so you must call this function every frame.
bool FBeatDetect::Detect( std::vector<float> & a_Samples, size_t a_nOffset )
{
	if ( a_nOffset >= (int)a_Samples.size() )
		throw WatsonException( "Buffer overflow" );
	if ( ((int)(a_Samples.size() - a_nOffset)) < m_nTimeSize )
		throw WatsonException( "Buffer underflow" );

	m_FFT.Forward( a_Samples, a_nOffset );

	float instant, E, V, C, diff, dAvg, diff2;
	for (int i = 0; i < (int)m_feBuffer.size(); i++)
	{
		instant = m_FFT.GetAverage(i);
		E = GetAverage(m_feBuffer[i]);
		V = GetVariance(m_feBuffer[i], E);
		C = (-0.0025714f * V) + 1.5142857f;
		diff = MAX(instant - C * E, 0);
		dAvg = GetSpecAverage(m_fdBuffer[i]);
		diff2 = MAX(diff - dAvg, 0);

		double now = Time().GetEpochTime();
		if ((now - m_Times[i]) < m_Sensitivity)
		{
			m_bIsOnset[i] = false;
		}
		else if (diff2 > 0)
		{
			m_bIsOnset[i] = true;
			m_Times[i] = now;
		}
		else
		{
			m_bIsOnset[i] = false;
		}
		m_feBuffer[i][m_nInsertAt] = instant;
		m_fdBuffer[i][m_nInsertAt] = diff;
	}

	m_nInsertAt++;
	if (m_nInsertAt == m_feBuffer[0].size())
		m_nInsertAt = 0;

	if ( m_nSubscriberCount > 0 )
		PublishSpectrum();

	return IsKick();
}
 
void FBeatDetect::InitializeTables()
{
	m_FFT.Initialize( m_nTimeSize, (float)m_fSampleRate );
	m_FFT.LogAverages(60, 3);
	int numAvg = m_FFT.GetAverageSize();
	m_bIsOnset.resize( numAvg );

	m_feBuffer.resize(numAvg);
	for(int i=0;i<(int)m_feBuffer.size();++i)
		m_feBuffer[i].resize( (size_t)(m_fSampleRate / m_nTimeSize) );
	m_fdBuffer.resize(numAvg);
	for(int i=0;i<(int)m_fdBuffer.size();++i)
		m_fdBuffer[i].resize( (size_t)(m_fSampleRate / m_nTimeSize) );

	m_Times.resize( numAvg );

	double start = Time().GetEpochTime();
	for (int i = 0; i < (int)m_Times.size(); i++)
		m_Times[i] = start;
	m_nInsertAt = 0;
}
void FBeatDetect::OnSubscriber( const ITopics::SubInfo & a_Info )
{
	if ( a_Info.m_Subscribed )
		m_nSubscriberCount += 1;
	else
		m_nSubscriberCount -= 1;
}

void FBeatDetect::PublishSpectrum()
{
	const int PIXELS_PER_BAND = 10;

	int width = m_FFT.GetAverageSize() * PIXELS_PER_BAND;
	int height = 100;

	float fMax = 50.0f;
	//for(size_t i=0;i<m_FFT.GetAverageSize();++i)
	//	fMax = MAX( fMax, m_FFT.GetAverage(i) );

	int length = (width * 3) * height;
	unsigned char * RGB = new unsigned char[ length ];
	memset( RGB, 0, length );

	for(int i=0;i< (int)m_FFT.GetAverageSize(); ++i)
	{
		float fBand = m_FFT.GetAverage(i) / fMax;
		if ( fBand > 1.0f )
			fBand = 1.0f;

		int left = i * PIXELS_PER_BAND;
		int right = left + PIXELS_PER_BAND;
		int top = (height - 1) - (int)((height - 1) * fBand);
		int bottom = height;

		if ( top == bottom )
			continue;			// 0, so just skip this band..

		for(int x=left;x<right;++x)
		{
			for(int y=top;y<bottom;++y)
			{
				int offset = (x * 3) + (y * (width * 3));
				RGB[ offset ] = (unsigned char)(0xff * fBand);		// red
				RGB[ offset + 1 ] = 0xff - RGB[offset];				// green
				if ( m_bIsOnset[i] )
					RGB[ offset + 2 ] = 0xff;						// beat detected
			}
		}
	}

	std::string jpeg;
	if ( JpegHelpers::EncodeImage( RGB, width, height, 3, jpeg ) )
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		if ( pInstance != NULL )
			pInstance->GetTopics()->Publish( "fbeatdetect", jpeg, false, true );
	}

	delete [] RGB;
}
