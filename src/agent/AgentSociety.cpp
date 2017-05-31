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


#include "AgentSociety.h"
#include "ProxyAgent.h"
#include "SelfInstance.h"

#include "utils/Log.h"
#include "IAgent.h"

AgentSociety::AgentSociety() : m_bActive( false )
{}

AgentSociety::~AgentSociety()
{}

bool AgentSociety::Start()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;
    if (m_bActive)
        return false;
    m_bActive = true;

	const SelfInstance::AgentList & agents = pInstance->GetAgentList();
    for (SelfInstance::AgentList::const_iterator iAgent = agents.begin(); iAgent != agents.end(); ++iAgent )
		AddAgent( *iAgent );

	m_pTopicManager = pInstance->GetTopicManager();
	m_pTopicManager->RegisterTopic("agent-society", "application/json",
		DELEGATE(AgentSociety, OnSubscriber, const ITopics::SubInfo &, this));
	m_pTopicManager->Subscribe("agent-society",
		DELEGATE(AgentSociety, OnAgentEvent, const ITopics::Payload &, this));

    Log::Status( "AgentSociety", "AgentSociety started." );
    return true;
}

bool AgentSociety::Stop()
{
    if (!m_bActive)
        return false;

	m_bActive = false;

    Log::Status( "AgentSociety", "Stopping AgentSociety." );
    //TODO: We will need to make sure that we free and stop each agent that has been spun up

    for (AgentList::iterator iAgent = m_Agents.begin();
         iAgent != m_Agents.end(); ++iAgent)
    {
        const IAgent::SP & spAgent = (*iAgent);
		if (!spAgent->IsActive())
			continue;

		Log::Debug( "AgentSociety", "Stopping agent %s...", spAgent->GetAgentName().c_str() );
		if (!spAgent->OnStop())
			Log::Error("AgentSociety", "Failed to stop agent %s.", spAgent->GetAgentName().c_str() );
		else
			Log::Debug( "AgentSociety", "... agent %s stopped.", spAgent->GetAgentName().c_str() );
    }
    m_Agents.clear();

    m_bActive = false;
    return true;
}

bool AgentSociety::FindAgentsByName(const std::string & a_Name, std::vector<IAgent::SP> & a_Overrides)
{
	for (AgentList::iterator iAgent = m_Agents.begin();
		iAgent != m_Agents.end(); ++iAgent)
	{
		if ((*iAgent)->GetAgentName().compare(a_Name) == 0)
			a_Overrides.push_back((*iAgent));
	}

	if (a_Overrides.size() > 0)
		return true;

	return false;
}

bool AgentSociety::AddAgent(const IAgent::SP & a_spAgent, bool a_bOveride)
{
	if (!a_spAgent)
		return false;

	a_spAgent->SetAgentSociety(this, a_bOveride);
	m_Agents.push_back(a_spAgent);

	if(m_bActive && a_spAgent->IsEnabled())
	{
		Log::Debug( "AgentSociety", "Starting agent %s...", a_spAgent->GetAgentName().c_str() );
		if (a_spAgent->OnStart())
		{
			a_spAgent->SetState(IAgent::AS_RUNNING);
			Log::Debug("AgentSociety", "... agent %s started.", a_spAgent->GetAgentName().c_str());
		}
		else
		{
			Log::Error("AgentSociety", "Failed to start agent %s.", a_spAgent->GetAgentName().c_str() );
		}
	}

	Log::Status("AgentSociety", "Added agent %s", a_spAgent->GetAgentName().c_str());

	return true;
}

bool AgentSociety::RemoveAgent(const IAgent::SP & a_spAgent)
{
	for (AgentList::iterator iAgent = m_Agents.begin();
		iAgent != m_Agents.end(); ++iAgent)
	{
		if ((*iAgent) == a_spAgent)
		{
			if (a_spAgent->IsActive())
				a_spAgent->OnStop();

			a_spAgent->SetAgentSociety(NULL, true);
			m_Agents.erase(iAgent);
			return true;
		}
	}

	return false;
}

void AgentSociety::OnAgentOverride(IAgent * a_pAgent)
{
	if (m_bActive)
	{
		if(a_pAgent->OnStop())
			a_pAgent->SetState(IAgent::AS_SUSPENDED);
		
	}
}

void AgentSociety::OnAgentOverrideEnd(IAgent * a_pAgent)
{
	if (m_bActive)
	{
		if(a_pAgent->OnStart())
			a_pAgent->SetState(IAgent::AS_RUNNING);
	}
}

void AgentSociety::OnSubscriber(const ITopics::SubInfo & a_Info)
{
	if (!a_Info.m_Subscribed)
	{
		for (size_t i = 0; i<m_Agents.size();)
		{
			ProxyAgent::SP spProxy = DynamicCast<ProxyAgent>(m_Agents[i]);
			++i;

			if (!spProxy)
				continue;

			if (spProxy->GetOrigin() == a_Info.m_Origin)
			{
				Log::Status("AgentSociety", "Removing proxy agent %s for origin %s",
					spProxy->GetAgentName().c_str(), a_Info.m_Origin.c_str());
				RemoveAgent(spProxy);

				i -= 1;		// back up one
			}
		}
	}
}

void AgentSociety::OnAgentEvent(const ITopics::Payload & a_Payload)
{
	if (a_Payload.m_RemoteOrigin[0] != 0)
	{
		Json::Reader reader(Json::Features::strictMode());

		Json::Value json;
		if (reader.parse(a_Payload.m_Data, json))
		{
			if (json["event"].isString())
			{
				const std::string & ev = json["event"].asString();
				if (ev == "add_agent_proxy")
				{
					const std::string & agentId = json["agentId"].asString();
					const std::string & agentName = json["name"].asString();

					bool bOverride = json["override"].asBool();

					Log::Status("AgentSociety", "Adding proxy agent %s, Id: %s, Override: %s",
						agentName.c_str(), agentId.c_str(), bOverride ? "True" : "False");

					ProxyAgent::SP spProxy(new ProxyAgent(agentName, agentId, bOverride, a_Payload.m_RemoteOrigin));
					AddAgent(spProxy, bOverride);
				}
				else if (ev == "remove_agent_proxy")
				{
					bool bSuccess = false;

					const std::string & agentId = json["agentId"].asString();
					for (AgentList::iterator iAgent = m_Agents.begin();
						iAgent != m_Agents.end(); ++iAgent)
					{
						if ((*iAgent)->GetGUID() == agentId)
						{
							Log::Status("AgentSociety", "Removing proxy agent %s, Id: %s",
								(*iAgent)->GetRTTI().GetName().c_str(), agentId.c_str());
							bSuccess = RemoveAgent((*iAgent));
							break;
						}
					}

					if (!bSuccess)
						Log::Warning("AgentSociety", "Failed to remove proxy agent %s", agentId.c_str());
				}
				else if (ev == "error")
				{
					const std::string & failed_event = json["failed_event"].asString();
					Log::Error("AgentSociety", "Received error on event: %s", failed_event.c_str());
				}
				else
				{
					Log::Warning("AgentSociety", "Received unknown event: %s", json.toStyledString().c_str());
				}
			}
		}
	}
}