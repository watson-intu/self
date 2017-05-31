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


#ifndef SELF_FOURIERFILTERS_H
#define SELF_FOURIERFILTERS_H

#include <complex>
#include <valarray>
#include <cmath>
#include <fstream>

#include "extractors/TextExtractor.h"
#include "SelfLib.h"

class SELF_API FourierFilters : public TextExtractor::IAudioFilter
{
public:
	RTTI_DECL();

	//! Types
	typedef std::complex<float>					ComplexNum;
	typedef std::valarray<ComplexNum> 			ComplexNumArray;

	//! Interface class for any audio filter that operates in the frequency domain
	class SELF_API IFourierFilter : public ISerializable,
		public boost::enable_shared_from_this<IFourierFilter>
	{
	public:
		RTTI_DECL();

		//! Types
		typedef boost::shared_ptr<IFourierFilter>		SP;
		typedef boost::weak_ptr<IFourierFilter>			WP;
		//! Interface
		virtual bool ApplyFilter( ComplexNumArray & a_Data ) = 0;		// return true if ComplexNumArray is modified
	};

	template<typename T>
	boost::shared_ptr<T> FindFilter() const
	{
		for (size_t i = 0; i < m_Filters.size(); ++i)
		{
			boost::shared_ptr<T> spFilter = DynamicCast<T>( m_Filters[i] );
			if (spFilter)
				return spFilter;
		}
		return boost::shared_ptr<T>();
	}

	//! Construction
	FourierFilters();

	//! ISerialziable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

    //! IAudioFilter interface
    virtual void ApplyFilter( SpeechAudioData & a_Data );

	//! Mutators
	void AddFilter( IFourierFilter::SP a_Filter );

private:
    typedef std::vector<IFourierFilter::SP>		Filters;

    //! Data
	Filters									m_Filters;
	int										m_LastInputSz;
	int										m_LastBitRate;
	int										m_NextPow2;

	//! Helpers for Background Noise Filtering
	void FFT(ComplexNumArray & raw_audio);
	void IFFT(ComplexNumArray & filtered_frequency_components);
	int NextPow2(int i);

	//! Debugging

    void SaveArrayToCSV(const ComplexNumArray & array, const char * filename);
};

#endif //SELF_FOURIERFILTERS_H
