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


#ifndef SELF_TEXT_DATA_H
#define SELF_TEXT_DATA_H

#include "IData.h"
#include "SelfLib.h"

//! This object wraps a simple string in the IData interface. This carries input from keyboards or local STT devices.
class SELF_API TextData : public IData 
{
public:
	RTTI_DECL();

	TextData()
	{}
	TextData(const std::string & a_Text, float a_fConfidence) 
		: m_Text( a_Text ), m_fConfidence( a_fConfidence )
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json)
	{
		json["m_Text"] = m_Text;
		json["m_fConfidence"] = m_fConfidence;
	}
	virtual void Deserialize(const Json::Value & json)
	{
		m_Text = json["m_Text"].asString();
		m_fConfidence = json["m_fConfidence"].asFloat();
	}

	//!Accessors
	const std::string & GetText() const
	{
		return m_Text;
	}
	float GetConfidence() const
	{
		return m_fConfidence;
	}
private:
	//!Data
	std::string  	m_Text;
	float			m_fConfidence;
};


#endif //TouchData
