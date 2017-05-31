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


#ifndef IWINDOWFUNCTION_H
#define IWINDOWFUNCTION_H

#include "utils/WatsonException.h"

#include <vector>

class IWindowFunction
{
public:
	//! Constants
	static const float PI;
	static const float TWO_PI;

	IWindowFunction() : m_Length( 0 )
	{}
	virtual ~IWindowFunction()
	{}

	virtual float Value(int a_Length, int a_Index ) = 0;

	//! Apply the windows to a portion of the sample buffer.
	void Apply( std::vector<float> & a_Samples, size_t a_Offset, size_t a_Length )
	{
		m_Length = a_Length;
		for(size_t n = a_Offset; n < a_Offset + a_Length;++n )
			a_Samples[n] *= Value( a_Length, n - a_Offset );
	}

	// Generates the curve of the window function.
	void GenerateCurve(size_t a_Length, std::vector<float> & a_Samples )
	{
		a_Samples.resize( a_Length );
		for (size_t n = 0; n < a_Length; n++) 
			a_Samples[n] = 1.0f * Value(a_Length, n);  
	}

private:
	//! Data
	size_t		m_Length;
};

#endif

