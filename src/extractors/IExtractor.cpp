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


#include "IExtractor.h"
#include "utils/Log.h"
#include "ProxyExtractor.h"
#include "FeatureManager.h"
#include "SelfInstance.h"


RTTI_IMPL(IExtractor, ISerializable);

void IExtractor::Serialize(Json::Value & json)
{
	json["m_bEnabled"] = m_bEnabled;
}

void IExtractor::Deserialize(const Json::Value & json)
{
	if ( json["m_bEnabled"].isBool() )
		m_bEnabled = json["m_bEnabled"].asBool();
}

void IExtractor::AddOverride()
{
	assert(m_Overrides >= 0);
	bool bDisableExtractor = m_Overrides == 0;
	m_Overrides += 1;
	if (bDisableExtractor && m_pFeatureManager != NULL)
	{
		m_pFeatureManager->OnFeatureExtractorOverride(this);
	}
}

void IExtractor::RemoveOverride()
{
	assert(m_Overrides > 0);
	m_Overrides -= 1;
	if (m_Overrides == 0 && m_pFeatureManager != NULL)
	{
		m_pFeatureManager->OnFeatureExtractorOverrideEnd(this);
	}
}

void IExtractor::SetFeatureManager(FeatureManager * a_pFeatureManager, bool a_bOverride)
{
	m_pFeatureManager = a_pFeatureManager;
	if (a_bOverride)
	{
		if (m_pFeatureManager != NULL)
		{
			assert(m_Overriden.size() == 0);
			if (m_pFeatureManager->FindExtractors(GetName(), m_Overriden))
			{
				for (size_t i = 0; i<m_Overriden.size(); ++i)
					m_Overriden[i]->AddOverride();
			}
		}
		else if (m_Overriden.size() > 0)
		{
			for (size_t i = 0; i<m_Overriden.size(); ++i)
				m_Overriden[i]->RemoveOverride();
			m_Overriden.clear();
		}
	}
}