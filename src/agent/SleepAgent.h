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


#ifndef SELF_SLEEP_AGENT_H
#define SELF_SLEEP_AGENT_H

#include "IAgent.h"
#include "SelfLib.h"

class SELF_API SleepAgent : public IAgent
{
public:
	RTTI_DECL();

	SleepAgent() : 
		m_WakeTime( 3 * (60 * 60) ), 
		m_SleepTime( 30 * 60 ), 
		m_bSleeping( false )
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Data
	double m_WakeTime;					// how many seconds we can run before we sleep
	double m_SleepTime;					// how long do we sleep
	bool m_bSleeping;

	std::vector<std::string> m_HealthSensorMasks;

	TimerPool::ITimer::SP m_spSleepTimer;
	TimerPool::ITimer::SP m_spWakeTimer;

	//! Event Handlers
	void 		OnHealth(const ThingEvent & a_ThingEvent);
	void		OnSleep();
	void		OnWake();
};


#endif // URL Agent
