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


#ifndef SELF_THINKINGAGENT_H
#define SELF_THINKINGAGENT_H

#include "IAgent.h"
#include "blackboard/IThing.h"
#include "blackboard/Status.h"
#include "utils/Factory.h"
#include "utils/TimerPool.h"
#include "sensors/IData.h"

#include "SelfLib.h"

//! Forward Declarations
class SelfInstance;
class Goal;

class SELF_API ThinkingAgent : public IAgent
{
public:
    RTTI_DECL();

    ThinkingAgent() : m_fProcessingTime( 2.0f )
    {
        m_PleaseWaitText.push_back( "ummmm...." );
    }

    //! ISerialize interface
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

    //! IAgent interface
    virtual bool OnStart();
    virtual bool OnStop();

private:
    //! Types
    struct ProcessingIThing
    {
        ProcessingIThing(ThinkingAgent * a_pAgent, IThing::SP a_spThing);
        ProcessingIThing(const ProcessingIThing & a_Copy);
        ~ProcessingIThing();

        void OnTimer();

        ThinkingAgent *     m_pAgent;
        IThing::SP          m_spThing;
		Status::SP			m_spStatus;
        TimerPool::ITimer::SP
							m_spTimer;
    };
    typedef boost::shared_ptr<ProcessingIThing> ProcessingIThingSP;
    typedef std::list< ProcessingIThingSP > ProcessingThingList;

    //! Data
    float					m_fProcessingTime;
    std::vector<std::string> m_PleaseWaitText;
    ProcessingThingList     m_ProcessingThingList;

    //! Callbacks
    virtual void OnIntent(const ThingEvent & a_ThingEvent);
    virtual void OnStatus(const ThingEvent & a_ThingEvent);
};

#endif //SELF_THINKINGAGENT_H