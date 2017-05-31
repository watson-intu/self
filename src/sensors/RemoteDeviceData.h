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


#ifndef SELF_REMOTEDEVICEDATA_H
#define SELF_REMOTEDEVICEDATA_H

#include "jsoncpp/json/json.h"

#include "IData.h"
#include "SelfLib.h"

//! This data type contains raw wave audio data..
class SELF_API RemoteDeviceData : public IData
{
public:
    RTTI_DECL();

	//! Construction
	RemoteDeviceData()
	{}
    RemoteDeviceData(const Json::Value & a_Content) : m_Content( a_Content )
    {}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json)
	{
		json["m_Content"] = m_Content;
	}
	virtual void Deserialize(const Json::Value & json)
	{
		m_Content = json["m_Content"];
	}
	
	//!Accessors
    const Json::Value & GetContent() const
    {
        return m_Content;
    }

private:
    //!Data
    Json::Value  	    m_Content;
};

#endif
