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


#include "IAgent.h"
#include "utils/Factory.h"
#include "utils/ThreadPool.h"
#include "blackboard/LearningIntent.h"

#include "SelfLib.h"

//! Forward Declarations
class SelfInstance;

#ifndef SELF_REMINDERAGENT_H
#define SELF_REMINDERAGENT_H

class SELF_API ReminderAgent : public IAgent
{
public:
    RTTI_DECL();

    //! Construction
    ReminderAgent() : m_Delay( 60.0f )
    {}

    //! ISerializable interface
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

    //! IAgent interface
    virtual bool OnStart();
    virtual bool OnStop();

	//! Accessors
	const std::vector<std::string> & GetSayings() const
	{
		return m_Sayings;
	}
	const std::map<std::string,std::string> & GetScheduleMap() const
	{
		return m_ScheduleMap;
	}

private:

    //! Data
    std::map<std::string, std::string>	m_ScheduleMap;
    float						        m_Delay;
    std::vector<std::string>            m_Sayings;

    TimerPool::ITimer::SP		        m_spScheduleTimer;

    //! Event Handlers
    void OnLearnedIntent(const ThingEvent &a_ThingEvent);
    void OnAddReminder(LearningIntent::SP a_spIntent);
    void OnCheckSchedule();
};

#endif //SELF_REMINDERAGENT_H
