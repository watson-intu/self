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


#ifndef SELF_GREETERAGENT_H
#define SELF_GREETERAGENT_H

#include "IAgent.h"
#include "blackboard/Person.h"
#include "blackboard/Text.h"
#include "utils/Factory.h"
#include "utils/TimerPool.h"

#include "SelfLib.h"


class SELF_API GreeterAgent : public IAgent
{
public:
    RTTI_DECL();

	GreeterAgent();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

    //! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

private:

	//! Types
	typedef std::list<Person::SP>			PersonList;
	typedef std::map<std::string,double>	GreetingMap;

	//! Data
	float		m_GreetTimeout;					// amount of time of silence to engage greeting
	float		m_SeenTimeout;					// how long until a person is timed out of the seen list
	std::string	m_GreetingIntent;
	GreetingMap	m_LastGreeting;					

    void		OnNewFace( const ThingEvent & a_ThingEvent );
	void		OnRecognizedFace( const ThingEvent & a_ThingEvent );
	void		OnSay( const ThingEvent & a_ThingEvent );
	void		OnPerson(const ThingEvent & a_ThingEvent);
};

#endif //SELF_GREETERAGENT_H
