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


#ifndef SELF_MUSIC_AGENT_H
#define SELF_MUSIC_AGENT_H

#include "IAgent.h"
#include "blackboard/Calculate.h"

#include "SelfLib.h"

class SELF_API MusicAgent : public IAgent
{
public:
	RTTI_DECL();

	MusicAgent() : m_DanceSkillId( "dance" )
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Data
	std::string		m_DanceSkillId;

	//! Callbacks
	void OnBeat(const ThingEvent & a_ThingEvent);
	void OnMusicStopped(const ThingEvent & a_ThingEvent);
};

#endif // SELF_MATHAGENT_H
