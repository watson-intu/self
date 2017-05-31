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


#ifndef SELF_WEATHERAGENT_H
#define SELF_WEATHERAGENT_H

#include "IAgent.h"
#include "blackboard/WeatherIntent.h"
#include "services/IWeather.h"
#include "services/ILocation.h"
#include "SelfLib.h"

class SELF_API WeatherAgent : public IAgent
{
public:
    RTTI_DECL();

	WeatherAgent() : 
		m_bFahrenheit (true),
        m_DefaultCity( "this city" ),
		m_WeatherOutOfRangeMessage( "I'm afraid I do not have a weather report for the day you mentioned. I can only check weather for the next 10 days" )
    {}

    //! ISerializable interface
    void Serialize( Json::Value & json );
    void Deserialize( const Json::Value & json );

    //! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Data
	bool					m_bFahrenheit;
    std::string             m_WeatherOutOfRangeMessage;
    std::string             m_DefaultCity;

	struct WeatherRequest
	{
		WeatherRequest(WeatherAgent * a_pAgent, const WeatherIntent::SP & a_spIntent );

		WeatherAgent *		m_pAgent;
		WeatherIntent::SP	m_spIntent;
		std::string			m_City;
		std::string			m_DateSpecified;
		float				m_Lat;
		float				m_Long;

		void				OnLocation(const Json::Value & json);
		void		        OnCurrentConditionsReceived( const Json::Value & json );
		void		        OnTenDayForecastReceived( const Json::Value & json );
	};

    //! Event Handlers
    void                    OnAnomaly(const ThingEvent & a_ThingEvent);
    void                    OnWeatherRequest(const ThingEvent & a_ThingEvent);
};

#endif //SELF_WEATHERAGENT_H
