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


#ifndef SELF_URL_AGENT_H
#define SELF_URL_AGENT_H

#include "IAgent.h"
#include "services/IBrowser.h"
#include "SelfLib.h"

//! This agent handles displaying the URL (e.g. WayBlazer)
//! It acts on a URL object placed on the blackboard. It sends the URL
//! using a service to a system for displaying the URL.

class SELF_API URLAgent : public IAgent
{
public:
	RTTI_DECL();

	URLAgent()
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Data
	std::vector<std::string> m_FailureResponses;

	//! Event Handlers
	void 		OnURL(const ThingEvent & a_ThingEvent);
	void        OnResponse( IBrowser::URLServiceData * a_UrlAgentData );

};


#endif // URL Agent
