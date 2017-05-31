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


#ifndef SELF_SOUND_GESTURE_H
#define SELF_SOUND_GESTURE_H

#include "IGesture.h"
#include "SelfLib.h"

//! This gesture wraps a sound effect to play back
class SELF_API SoundGesture : public IGesture
{
public:
	RTTI_DECL();

	//! Construction
	SoundGesture() : m_fVolume( 1.0f ), m_fPan( 0.0f )
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IGesture interface
	virtual bool Execute( GestureDelegate a_Callback, const ParamsMap & a_Params );
	virtual bool Abort();

	const std::string & GetSound() const
	{
		return m_Sound;
	}

	void SetSound( const std::string & a_Sound )
	{
		m_Sound = a_Sound;
	}

protected:
	//!Data
	std::string			m_Sound;		// path to the sound to play
	float				m_fVolume;
	float				m_fPan;
};


#endif //SELF_SOUND_GESTURE_H
