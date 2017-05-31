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


#include "utils/Log.h"
#include "FeatureManager.h"
#include "ProxyExtractor.h"
#include "SelfInstance.h"


FeatureManager::FeatureManager() : m_bActive( false )
{}

FeatureManager::~FeatureManager()
{}

bool FeatureManager::Start()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;
	if (m_bActive)
        return false;
    m_bActive = true;

	const FeatureExtractorList & extractors = pInstance->GetExtractorList();
	for (FeatureExtractorList::const_iterator iExtractor = extractors.begin();
		iExtractor != extractors.end(); ++iExtractor)
	{
		AddFeatureExtractor(*iExtractor);
	}

	m_pTopicManager = SelfInstance::GetInstance()->GetTopicManager();
	m_pTopicManager->RegisterTopic("feature-manager", "application/json",
		DELEGATE(FeatureManager, OnSubscriber, const ITopics::SubInfo &, this));
	m_pTopicManager->Subscribe("feature-manager",
		DELEGATE(FeatureManager, OnFeatureExtractorEvent, const ITopics::Payload &, this));

    Log::Status( "FeatureManager", "FeatureManager started, %d feature extractors", m_FeatureExtractors.size() );
    return true;
}

bool FeatureManager::Stop()
{
    if (!m_bActive)
        return false;

    Log::Status( "FeatureManager", "Stopping FeatureManager." );
    for (FeatureExtractorList ::iterator iExtractor = m_FeatureExtractors.begin();
         iExtractor != m_FeatureExtractors.end(); ++iExtractor)
    {
        IExtractor::SP spFeatureExtractor = *iExtractor;
        if (! spFeatureExtractor->OnStop() )
			Log::Error( "FeatureManager", "Failed to stop feature extractor %s", spFeatureExtractor->GetName() );
    }
    m_FeatureExtractors.clear();

    m_bActive = false;
    return true;
}

bool FeatureManager::AddFeatureExtractor(const IExtractor::SP & a_spExtractor, bool a_bOveride /*= false*/)
{
	if (!a_spExtractor)
		return false;

	a_spExtractor->SetFeatureManager(this, a_bOveride);
	m_FeatureExtractors.push_back(a_spExtractor);

	if (m_bActive && a_spExtractor->IsEnabled() )
	{
		if (a_spExtractor->OnStart())
			a_spExtractor->SetState(IExtractor::AS_RUNNING);
	}

	Log::Status("FeatureExtractor", "Added extractor %s", a_spExtractor->GetRTTI().GetName().c_str());

	return true;
}

bool FeatureManager::RemoveFeatureExtractor(const IExtractor::SP & a_spFeatureExtractor)
{
	for (FeatureExtractorList::iterator iFeatureExtractor = m_FeatureExtractors.begin();
		iFeatureExtractor != m_FeatureExtractors.end(); ++iFeatureExtractor)
	{
		if ((*iFeatureExtractor) == a_spFeatureExtractor)
		{
			if (a_spFeatureExtractor->IsActive())
				a_spFeatureExtractor->OnStop();

			a_spFeatureExtractor->SetFeatureManager(NULL, true);
			m_FeatureExtractors.erase(iFeatureExtractor);
			return true;
		}
	}

	return false;
}

void FeatureManager::OnFeatureExtractorOverride(IExtractor * a_pFeatureExtractor)
{
	if (m_bActive)
	{
		if (a_pFeatureExtractor->OnStop())
			a_pFeatureExtractor->SetState(IExtractor::AS_SUSPENDED);

	}
}

void FeatureManager::OnFeatureExtractorOverrideEnd(IExtractor * a_pFeatureExtractor)
{
	if (m_bActive)
	{
		if (a_pFeatureExtractor->OnStart())
			a_pFeatureExtractor->SetState(IExtractor::AS_RUNNING);
	}
}

bool FeatureManager::FindExtractors(const std::string & a_Name, std::vector<IExtractor::SP> & a_Overrides)
{
	for (FeatureExtractorList::iterator iFeatureExtractor = m_FeatureExtractors.begin();
		iFeatureExtractor != m_FeatureExtractors.end(); ++iFeatureExtractor)
	{
		if ((*iFeatureExtractor)->GetName() == a_Name )
			a_Overrides.push_back((*iFeatureExtractor));
	}

	if (a_Overrides.size() > 0)
		return true;

	return false;
}

void FeatureManager::OnSubscriber(const ITopics::SubInfo & a_Info)
{
	if (!a_Info.m_Subscribed)
	{
		for (FeatureExtractorList::iterator iFeatureExtractor = m_FeatureExtractors.begin();
			iFeatureExtractor != m_FeatureExtractors.end();)
		{
			ProxyExtractor::SP spProxy = DynamicCast<ProxyExtractor>(*iFeatureExtractor++);

			if (!spProxy)
				continue;

			if (spProxy->GetOrigin() == a_Info.m_Origin)
			{
				Log::Status("FeatureManager", "Removing proxy extractor %s for origin %s",
					spProxy->GetGUID().c_str(), a_Info.m_Origin.c_str());
				RemoveFeatureExtractor(spProxy);
			}
		}
	}
}

void FeatureManager::OnFeatureExtractorEvent(const ITopics::Payload & a_Payload)
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
				if (ev == "add_extractor_proxy")
				{
					const std::string & extractorId = json["extractorId"].asString();
					const std::string & extractorName = json["name"].asString();

					bool bOverride = json["override"].asBool();

					Log::Status("FeatureManager", "Adding proxy extractor %s, Id: %s, Override: %s",
						extractorName.c_str(), extractorId.c_str(), bOverride ? "True" : "False");

					ProxyExtractor::SP spProxy(new ProxyExtractor(extractorName, extractorId, bOverride, a_Payload.m_RemoteOrigin));
					AddFeatureExtractor(spProxy, bOverride);
				}
				else if (ev == "remove_extractor_proxy")
				{
					bool bSuccess = false;

					const std::string & extractorId = json["extractorId"].asString();
					for (FeatureExtractorList::iterator iFeatureExtractor = m_FeatureExtractors.begin();
						iFeatureExtractor != m_FeatureExtractors.end(); ++iFeatureExtractor)
					{
						if ((*iFeatureExtractor)->GetGUID() == extractorId)
						{
							Log::Status("FeatureManager", "Removing proxy extractor %s, Id: %s",
								(*iFeatureExtractor)->GetName(), extractorId.c_str());
							bSuccess = RemoveFeatureExtractor((*iFeatureExtractor));
							break;
						}
					}

					if (!bSuccess)
						Log::Warning("AgentSociety", "Failed to remove proxy agent %s", extractorId.c_str());
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


