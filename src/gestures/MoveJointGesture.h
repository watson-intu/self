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


#ifndef MOVEJOINTGESTURE_H
#define MOVEJOINTGESTURE_H

#include "IGesture.h"
#include "SelfLib.h"

//! This is the base class for any gesture that moves a single joint on a robot.
class SELF_API MoveJointGesture : public IGesture
{
public:
	RTTI_DECL();

	//! Construction
	MoveJointGesture() : m_fStiffness( 0.0f ), m_bAbsolute( false )
	{}
	MoveJointGesture( const std::string & a_GestureId ) : IGesture( a_GestureId ),
		m_fStiffness( 0.0f ), m_bAbsolute( false )
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IGesture interface
	virtual bool Execute( GestureDelegate a_Callback, const ParamsMap & a_Params );

	//! Mutators
	void SetStiffness(float a_fStiffness)
	{
		m_fStiffness = a_fStiffness;
	}
	void SetAsbsolute( bool a_bAbsolute )
	{
		m_bAbsolute = a_bAbsolute;
	}

protected:
	//! Data
	std::vector<std::string> m_JointNames;
	std::vector<float> m_fAngles;
	std::vector<float> m_fSpeeds;
	float m_fStiffness;
	bool m_bAbsolute;
};


#endif //IGESTURE_H
