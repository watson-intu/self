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


#ifndef SELF_NOISEFILTER_H
#define SELF_NOISEFILTER_H

#include <complex>
#include <valarray>
#include <cmath>
#include <fstream>

#include "FourierFilters.h"
#include "SelfLib.h"

class SELF_API NoiseFilter : public FourierFilters::IFourierFilter
{
public:
	RTTI_DECL();

	//! Types
	typedef std::complex<float>				ComplexNum;
	typedef std::valarray<ComplexNum> 		ComplexNumArray;
	typedef std::valarray<float>			NumArray;

	//! Construction
	NoiseFilter();

	//! ISerialziable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

    //! IFourierFilter interface
    virtual bool ApplyFilter( ComplexNumArray & a_X );

private:
    //! Data
	NumArray							 	m_AvgFourierMagnitudes;
	int 									m_WindowSz;
	int										m_Count;
	bool									m_bPrimed;
	float									m_FiltCoeff;
};

#endif //SELF_NOISEFILTER_H
