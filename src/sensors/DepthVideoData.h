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


#ifndef SELF_DEPTH_VIDEODATA_H
#define SELF_DEPTH_VIDEODATA_H

#include <vector>
#include <cstring>
#include "IData.h"
#include "SelfLib.h"

//! This class holds png encoded depth data 
class SELF_API DepthVideoData : public IData
{
public:
	RTTI_DECL();

	DepthVideoData()
	{}
	DepthVideoData(std::vector<unsigned char> a_BinaryData) :
		m_BinaryData((const char *)&a_BinaryData[0], a_BinaryData.size())
	{}
	DepthVideoData( const std::string & a_BinaryData ) :
		m_BinaryData( a_BinaryData )
	{}
	DepthVideoData(const unsigned char * a_pBinaryData, int a_BinaryLength) :
		m_BinaryData((const char *)a_pBinaryData, a_BinaryLength)
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json)
	{
		json["m_BinaryData"] = StringUtil::EncodeBase64( m_BinaryData );
	}
	virtual void Deserialize(const Json::Value & json)
	{
		m_BinaryData = StringUtil::DecodeBase64( json["m_BinaryData"].asString() );
	}

	//! IData interface
	virtual bool ToBinary(std::string & a_Output)
	{
		a_Output = m_BinaryData;
		return true;
	}
	virtual bool FromBinary( const std::string & a_Type, const std::string & a_Input)
	{
		m_BinaryData = a_Input;
		return true;
	}

	//!Accessors
	const std::string & GetBinaryData() const
	{
		return m_BinaryData;
	}

private:
	//!Data
	std::string			m_BinaryData;
};

#endif //SELF_DEPTH_VIDEODATA_H
