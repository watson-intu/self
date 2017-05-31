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
#include "AgentSociety.h"
#include "utils/UniqueID.h"

RTTI_IMPL(IAgent, ISerializable);

void IAgent::Serialize(Json::Value & json)
{
	json["m_bEnabled"] = m_bEnabled;
}

void IAgent::Deserialize(const Json::Value & json)
{
	if (json["m_bEnabled"].isBool())
		m_bEnabled = json["m_bEnabled"].asBool();
}

void IAgent::AddOverride()
{
	assert(m_Overrides >= 0);
	bool bDisableAgent = m_Overrides == 0;
	m_Overrides += 1;

	if (bDisableAgent && m_pSociety != NULL)
		m_pSociety->OnAgentOverride(this);
}

void IAgent::RemoveOverride()
{
	assert(m_Overrides > 0);
	m_Overrides -= 1;

	if (m_Overrides == 0 && m_pSociety != NULL)
		m_pSociety->OnAgentOverrideEnd(this);
}

void IAgent::SetAgentSociety(AgentSociety * a_pSociety, bool a_bOverride)
{
	m_pSociety = a_pSociety;
	if (m_bEnabled && a_bOverride)
	{
		if (m_pSociety != NULL)
		{
			assert(m_Overriden.size() == 0);
			if (m_pSociety->FindAgentsByName(GetAgentName(), m_Overriden))
			{
				for (size_t i = 0; i < m_Overriden.size(); ++i)
					m_Overriden[i]->AddOverride();
			}
		}
		else if (m_Overriden.size() > 0)
		{
			for (size_t i = 0; i < m_Overriden.size(); ++i)
				m_Overriden[i]->RemoveOverride();
			m_Overriden.clear();
		}
	}
}