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


#ifndef SELF_MOODDATA_H
#define SELF_MOODDATA_H

#include "SelfLib.h"

class SELF_API MoodData : public IData
{
public:
    RTTI_DECL();

	//! Construction
    MoodData(bool a_IsSmiling = false)
            : m_IsSmiling(a_IsSmiling)
    {}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json)
	{
		json["m_IsSmiling"] = m_IsSmiling;
	}
	virtual void Deserialize(const Json::Value & json)
	{
		m_IsSmiling = json["m_IsSmiling"].asBool();
	}
	
	//!Accessors
    bool GetIsSmiling() const
    {
        return m_IsSmiling;
    }

private:
    //!Data
    bool			m_IsSmiling;
};

#endif //SELF_MOODDATA_H
