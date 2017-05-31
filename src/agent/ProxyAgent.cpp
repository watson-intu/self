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


#include "ProxyAgent.h"
#include "SelfInstance.h"
#include "AgentSociety.h"
#include "utils/UniqueID.h"
#include "topics/ITopics.h"

RTTI_IMPL(ProxyAgent, IAgent);
REG_SERIALIZABLE(ProxyAgent);

ProxyAgent::ProxyAgent(const std::string & a_AgentName,
	const std::string & a_InstanceId,
	bool a_bOverride, const std::string & a_Origin) :
	m_AgentName(a_AgentName),
	m_Origin(a_Origin),
	m_bOverride(a_bOverride)
{
	SetGUID(a_InstanceId);
}

ProxyAgent::ProxyAgent() :
	m_bOverride(false)
{}

ProxyAgent::~ProxyAgent()
{}

void ProxyAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
	json["m_AgentName"] = m_AgentName;
	json["m_Origin"] = m_Origin;
	json["m_bOverride"] = m_bOverride;
}

void ProxyAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
	m_AgentName = json["m_AgentName"].asString();
	m_Origin = json["m_Origin"].asString();
	m_bOverride = json["m_bOverride"].asBool();
}

bool ProxyAgent::OnStart()
{
	SendEvent("start_agent");
	return true;
}

bool ProxyAgent::OnStop()
{
	SendEvent("stop_agent");
	return true;
}

void ProxyAgent::SendEvent(const std::string & a_EventName)
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert(pInstance != NULL);
	ITopics * pTopics = pInstance->GetTopics();
	assert(pTopics != NULL);

	Json::Value ev;
	ev["event"] = a_EventName;
	ev["agentId"] = GetGUID();

	pTopics->Send(m_Origin, "agent-society", ev.toStyledString());
}
