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


#ifndef SELF_TIMEAGENT_H
#define SELF_TIMEAGENT_H

#include "IAgent.h"
#include "blackboard/TimeIntent.h"
#include "SelfLib.h"

#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

class SELF_API TimeAgent : public IAgent
{
public:
    RTTI_DECL();

    TimeAgent() : m_TimeZoneDB( "shared/date_time_zonespec.csv" )
    {}

    //! ISerializable interface
    void Serialize( Json::Value & json );
    void Deserialize( const Json::Value & json );

    //! IAgent interface
    virtual bool OnStart();
    virtual bool OnStop();

private:
    //! Data
    std::string			m_TimeZone;
    std::string         m_TimeZoneDB;
    std::string         m_TimeTag;
	TimeIntent::SP		m_spActive;
    boost::local_time::tz_database	
						m_TZdb;

    //! Event Handlers
    void				OnTimeIntent(const ThingEvent & a_ThingEvent);
    void				OnTime(const Json::Value & json);
	void				OnTimeZone(const Json::Value & json);
	void				OnExecute(const std::string & a_TimeZone);
};

#endif //SELF_TIMEAGENT_H
