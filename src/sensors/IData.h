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


#ifndef SELF_IDATA_H
#define SELF_IDATA_H

#include "utils/RTTI.h"
#include "utils/ISerializable.h"
#include "utils/StringUtil.h"
#include "SelfLib.h"

class ISensor;

//! This is the base class for any type of data sent by a sensor
class SELF_API IData : public ISerializable
{
public:
	RTTI_DECL();

	IData() : m_pOrigin( NULL )
	{}
	virtual ~IData()
	{}

	ISensor * GetOrigin() const 
	{
		return m_pOrigin;
	}
	void SetOrigin( ISensor * a_pSensor )
	{
		m_pOrigin = a_pSensor;
	}

	//! By default, the ToBinary/FromBinary just convert the data to and from Json using the ISerialiable interface
	//! we can expect certain data types to override this an actually serialize real binary data.
	virtual bool ToBinary( std::string & a_Output )
	{
		a_Output = ISerializable::SerializeObject( this ).toStyledString();
		return true;
	}
	virtual bool FromBinary( const std::string & a_Type, const std::string & a_Input )
	{
		if ( StringUtil::Compare( a_Type,"application/json", true ) == 0 )
			return ISerializable::DeserializeObject( a_Input, this ) != NULL;
		return false;
	}

private:
	//! Data
	ISensor *		m_pOrigin;
};

#endif
