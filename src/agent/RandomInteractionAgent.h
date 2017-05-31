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


#ifndef SELF_RANDOMINTERACTIONAGENT_H
#define SELF_RANDOMINTERACTIONAGENT_H

#include <list>

#include "IAgent.h"
#include "utils/Factory.h"
#include "utils/ThreadPool.h"

#include "SelfLib.h"


//! Forward Declarations
class SelfInstance;

class SELF_API RandomInteractionAgent : public IAgent
{
public:
	RTTI_DECL();

	//! Construction
	RandomInteractionAgent() : 
		m_fMinSpeakDelay( 60.0f ), 
		m_fMaxSpeakDelay( 120.0f ),
		m_RandomInteractionUtterance("random_interaction")
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

	void ResetTimeDelays( const float & a_fMinSpeakDelay, const float & a_fMaxSpeakDelay )
	{
		m_fMinSpeakDelay = a_fMinSpeakDelay;
		m_fMaxSpeakDelay = a_fMaxSpeakDelay;
		StartTimer();
	}

private:
	//! Data
	float						m_fMinSpeakDelay;
	float						m_fMaxSpeakDelay;
	std::string					m_RandomInteractionUtterance;

	TimerPool::ITimer::SP		m_spSpeakTimer;

	//! Event Handlers
	void OnInteraction(const ThingEvent &a_ThingEvent);
	void StartTimer();
	void OnSpeak();
};

#endif //SELF_RANDOMINTERACTIONAGENT_H
