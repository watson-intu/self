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


#ifndef GUASS_WINDOW_H
#define GUASS_WINDOW_H

#include "IWindowFunction.h"
#include "utils/WatsonException.h"

class GaussWindow : public IWindowFunction
{
public:
	GaussWindow( double a_Alpha = 0.25 ) : m_Alpha( a_Alpha )	
	{
		if ( m_Alpha > 0.5 || m_Alpha < 0.0 )
			throw new WatsonException( "Range for GaussWindow out of bounds. Value must be <= 0.5" );
	}

	float Value(int length, int index) 
	{
		return (float) pow(2.718, -0.5 * pow((index - (length - 1) / (double) 2) / (m_Alpha * (length - 1) / (double) 2), (double) 2));
	}

private:
	//! Data
	double m_Alpha;
};

#endif
