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


#ifndef SELF_GESTURE_H
#define SELF_GESTURE_H

#include "IThing.h"

//! This object is placed on the blackboard when we need self to say anything
class SELF_API Gesture : public IThing
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<Gesture>			SP;
	typedef boost::weak_ptr<Gesture>			WP;

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Construction
	Gesture() : IThing(TT_COGNITIVE, 30.0f)
	{}
	Gesture(const std::string & a_Type) : 
		m_Type(a_Type), IThing(TT_COGNITIVE, 30.0f)
	{}
	Gesture(const std::string & a_Type, const ParamsMap & a_Params ) : 
		m_Type(a_Type), m_Params( a_Params ), IThing(TT_COGNITIVE, 30.0f)
	{}

	//! Accessors
	const std::string & GetType() const
	{
		return m_Type;
	}
	const ParamsMap & GetParams() const
	{
		return m_Params;
	}

	void SetType(const std::string & a_Type)
	{
		m_Type = a_Type;
	}
	void SetParams( const ParamsMap & a_Param )
	{
		m_Params = a_Param;
	}

private:
	//! Data
	std::string m_Type;
	ParamsMap m_Params;
};

#endif //SELF_GESTURE_H

