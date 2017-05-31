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


#ifndef HEALTH_DATA_H
#define HEALTH_DATA_H

#include "jsoncpp/json/json.h"

#include "IData.h"
#include "SelfLib.h"

class SELF_API HealthData : public IData
{
public:
    RTTI_DECL();

	//! Construction
	HealthData()
	{}
	HealthData( const std::string & a_HealthName, const Json::Value & a_Content ) :
		m_HealthName( a_HealthName ), m_Content( a_Content )
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json)
	{
		json["m_HealthName"] = m_HealthName;
		json["m_Content"] = m_Content;
	}
	virtual void Deserialize(const Json::Value & json)
	{
		m_HealthName = json["m_HealthName"].asString();
		m_Content = json["m_Content"];
	}

    //!Accessors
    const std::string & GetHealthName() const
    {
        return m_HealthName;
    }

	Json::Value & GetContent()
	{
		return m_Content;
	}

	//!Mutators
	void SetHealthName( std::string & a_HealthName )
	{
		m_HealthName = a_HealthName;
	}

private:
    //!Data
    std::string     m_HealthName;  // name of service, joint, etc
	Json::Value     m_Content;
};


#endif //HealthData
