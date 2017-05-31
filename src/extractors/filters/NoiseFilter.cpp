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


#include "NoiseFilter.h"

#include <iostream>
#include <fstream>

REG_SERIALIZABLE(NoiseFilter);
RTTI_IMPL(NoiseFilter, IFourierFilter);

NoiseFilter::NoiseFilter()
{
    m_WindowSz = 500;
    m_AvgFourierMagnitudes = NumArray(0);
    m_Count = 0;
    m_bPrimed = false;
    m_FiltCoeff = 0.3f;
}

void NoiseFilter::Serialize(Json::Value & json)
{
    json["m_WindowSz"] = m_WindowSz;
    json["m_FiltCoeff"] = m_FiltCoeff;
}

void NoiseFilter::Deserialize(const Json::Value & json)
{
    if ( json.isMember("m_WindowSz") )
        m_WindowSz = json["m_WindowSz"].asInt();
    if ( json.isMember("m_FiltCoeff") )
        m_FiltCoeff = json["m_FiltCoeff"].asFloat();
}

bool NoiseFilter::ApplyFilter( ComplexNumArray & a_X )
{
    const size_t sz = a_X.size();
    
    if ( sz / 2 != m_AvgFourierMagnitudes.size() )
    {
        m_AvgFourierMagnitudes.resize( size_t(sz / 2) );
        Log::Debug("NoiseFilter", "Resizing average frequency array to %d... (%d)", sz / 2, m_AvgFourierMagnitudes.size() );
        m_Count = 0;
        m_bPrimed = false;        
    }

    // Running average
	for (size_t i = 1; i < m_AvgFourierMagnitudes.size(); i++)
	{
        if ( m_bPrimed )
            m_AvgFourierMagnitudes[i] -= m_AvgFourierMagnitudes[i] / m_WindowSz;
        m_AvgFourierMagnitudes[i] += std::abs( a_X[i] ) / m_WindowSz;
       
        if (m_AvgFourierMagnitudes[i] < 0 )
            Log::Debug("NoiseFilter", "Fourier Magnitude under 0 : %.3f", m_AvgFourierMagnitudes[i] );

	}

	// Spectral Subtraction
	for (size_t i = 1; i < m_AvgFourierMagnitudes.size(); i++)
	{
		// Magnitude subtraction
        float mag = std::abs( a_X[i] );
        mag -= m_FiltCoeff*m_AvgFourierMagnitudes[i];
        mag = mag < 0 ? 0 : mag;

        // Reconstruct complex
        a_X[i] = std::polar( mag, std::arg(a_X[i]) );  
        a_X[sz - 1 - i ] = std::polar( mag, std::arg( a_X[sz - 1 - i] ) );
	}

    // To add: binning and restricted band for filtering around human voice

    if (! m_bPrimed )
    {
        m_Count++;
        m_bPrimed = m_Count > m_WindowSz;
        if (m_bPrimed)
            Log::Debug("NoiseFilter", "Background Noise Filter primed");
    }

	return true;
}
