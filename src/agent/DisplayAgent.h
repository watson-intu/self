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


#ifndef SELF_DISPLAYAGENT_H
#define SELF_DISPLAYAGENT_H

#include "IAgent.h"
#include "blackboard/Display.h"
#include "SelfLib.h"

class SkillInstance;

class SELF_API DisplayAgent : public IAgent
{
public:
    RTTI_DECL();

    DisplayAgent();

    //! ISerializable interface
    void Serialize( Json::Value & json );
    void Deserialize( const Json::Value & json );

    //! IAgent interface
    virtual bool OnStart();
    virtual bool OnStop();

private:
    //! Types
    typedef std::list<Display::SP>		DisplayList;
    //! Event Handlers
    void                    OnDisplay(const ThingEvent & a_ThingEvent);
    void                    ExecuteDisplay(Display::SP a_pDisplay);
    void                    OnSkillState( SkillInstance * a_pInstance );

    std::string             m_DisplaySkill;
    Display::SP	            m_spActive;
    DisplayList             m_Displays;

};

#endif //SELF_DISPLAYAGENT_H
