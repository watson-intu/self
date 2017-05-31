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


#ifndef SELF_AGENTSOCIETY_H
#define SELF_AGENTSOCIETY_H

#include <list>

#include "IAgent.h"
#include "topics/TopicManager.h"
#include "utils/Factory.h"
#include "utils/RTTI.h"
#include "SelfLib.h"		// include last

class SelfInstance;
class ProxySensor;

class SELF_API AgentSociety
{
public:
    //! Types
    typedef std::vector< IAgent::SP >	AgentList;
    typedef Factory< IAgent >			AgentFactory;

    //! Construction
    AgentSociety();
    ~AgentSociety();

    //! Accessors
    const AgentList &		    GetAgentList() const;

	//! Start this manager
    bool						Start();
    //! Stop this manager.
    bool						Stop();

	//! Add the agent to this society, it takes ownership of the object if accepted.
	bool					AddAgent(const IAgent::SP & a_spAgent, bool a_bOverride = false);
	//! Remove a agent from this society.
	bool					RemoveAgent(const IAgent::SP & a_spAgent);

	bool					FindAgentsByName(const std::string & a_Name, std::vector<IAgent::SP> & a_Overrides);


	void OnAgentOverride(IAgent * a_pAgent);
	void OnAgentOverrideEnd(IAgent * a_pAgent);

	template<typename T>
	T * GetAgent() const
	{
		for( AgentList::const_iterator iAgent = m_Agents.begin(); iAgent != m_Agents.end(); ++iAgent )
		{
			T * pAgent = DynamicCast<T>( *iAgent );
			if ( pAgent != NULL )
				return pAgent;
		}
		return NULL;
	}

private:
    //! Data
    bool						m_bActive;
    AgentList				    m_Agents;
	TopicManager *				m_pTopicManager;

	//! Callbacks
	void					OnSubscriber(const ITopics::SubInfo & a_Info);
	void					OnAgentEvent(const ITopics::Payload & a_Payload);

};

inline const AgentSociety::AgentList & AgentSociety::GetAgentList() const
{
    return m_Agents;
}

#endif //SELF_AGENTSOCIETY_H