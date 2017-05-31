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


#ifndef MOVE_GESTURE_H
#define MOVE_GESTURE_H

#include "IGesture.h"
#include "SelfLib.h"

//! This gesture moves the robot to a location in the enviroment.
class SELF_API MoveGesture : public IGesture
{
public:
	RTTI_DECL();

	//! Construction
	MoveGesture() : m_fX( 0.0f ), m_fY( 0.0f ), m_fZ( 0.0f )
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IGesture interface
	virtual bool Execute( GestureDelegate a_Callback, const ParamsMap & a_Params );
	virtual bool Abort();

protected:
	//! Data
	float m_fX;
	float m_fY;
	float m_fZ;
};


#endif //GRASP_GESTURE_H
