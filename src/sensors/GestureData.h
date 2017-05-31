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


#ifndef SELF_GESTUREDATA_H
#define SELF_GESTUREDATA_H

#include "IData.h"
#include "SelfLib.h"

class SELF_API GestureData : public IData
{
public:
    RTTI_DECL();

	GestureData()
	{}
    GestureData(const std::string & a_Gesture)
            : m_Gesture(a_Gesture)
    {}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json)
	{
		json["m_Gesture"] = m_Gesture;
	}
	virtual void Deserialize(const Json::Value & json)
	{
		m_Gesture = json["m_Gesture"].asString();
	}

	//!Accessors
    const std::string & GetGesture() const
    {
        return m_Gesture;
    }

    void SetGesture(const std::string & a_Gesture)
    {
        m_Gesture = a_Gesture;
    }

private:

    //!Data
    std::string	m_Gesture;
};

#endif //SELF_GESTUREDATA_H
