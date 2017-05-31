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


#ifndef SELF_TOUCH_H
#define SELF_TOUCH_H

#include "IThing.h"

//! This is the Touch object that gets put onto the blackboard
class SELF_API Touch : public IThing
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<Touch>		SP;
	typedef boost::weak_ptr<Touch>			WP;

	//! Construction
	Touch() : IThing(TT_COGNITIVE)
	{}
	

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Accessors and mutators
	const std::string & GetSensorName() const
	{
		return m_SensorName;
	}

	float GetEngaged() const
	{
		return m_fEngaged;
	}

	bool IsEngaged() const
	{
		return m_fEngaged > 0.5f;
	}

	void SetEngage(float a_fEngaged)
	{
		m_fEngaged = a_fEngaged;
	}

	void SetSensorName(const std::string & a_SensorName)
	{
		m_SensorName = a_SensorName;
	}


private:
	//! Data
	std::string         m_SensorName;
	float				m_fEngaged;
};

#endif // Touch
