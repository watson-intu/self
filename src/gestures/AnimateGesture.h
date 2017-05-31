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


#ifndef ANIMATE_GESTURE_H
#define ANIMATE_GESTURE_H

#include "IGesture.h"
#include "SelfLib.h"

//! This gesture animates a robot using a built in animation ID
class SELF_API AnimateGesture : public IGesture
{
public:
	RTTI_DECL();

	//! Construction
	AnimateGesture()
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IGesture interface
	virtual bool Execute( GestureDelegate a_Callback, const ParamsMap & a_Params );
	virtual bool Abort();

protected:
	struct AnimationEntry
	{
		AnimationEntry( const std::string & a_AnimId, const std::string & a_RequiredPosture ) :
			m_AnimId( a_AnimId ), m_RequiredPosture( a_RequiredPosture )
		{}

		std::string m_AnimId;
		std::string m_RequiredPosture;
	};
	std::vector<AnimationEntry> m_Animations;
};


#endif //ANIMATE_GESTURE_H
