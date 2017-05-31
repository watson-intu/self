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
#include "ClassifierManager.h"
#include "ProxyClassifier.h"
#include "SelfInstance.h"

ClassifierManager::ClassifierManager() : m_bActive( false )
{}

ClassifierManager::~ClassifierManager()
{}

bool ClassifierManager::Start()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;
	if (m_bActive)
		return false;
	m_bActive = true;

	// instantiate all classifiers
	const ClassifierList & classifiers = pInstance->GetClassifierList();
	for( ClassifierList::const_iterator iClassifier = classifiers.begin(); 
		iClassifier != classifiers.end(); ++iClassifier )
	{
		AddClassifier( *iClassifier );
	}

	m_pTopicManager = SelfInstance::GetInstance()->GetTopicManager();
	m_pTopicManager->RegisterTopic("classifier-manager", "application/json",
		DELEGATE(ClassifierManager, OnSubscriber, const ITopics::SubInfo &, this));
	m_pTopicManager->Subscribe("classifier-manager",
		DELEGATE(ClassifierManager, OnClassifierEvent, const ITopics::Payload &, this));

	Log::Status( "ClassifierManager", "ClassifierManager started, %d classifiers created.", m_Classifiers.size() );
	return true;
}

bool ClassifierManager::Stop()
{
	if (!m_bActive)
		return false;

	Log::Status( "ClassifierManager", "Stopping ClassifierManager." );
	for (ClassifierList::iterator iClassifier = m_Classifiers.begin();
		iClassifier != m_Classifiers.end(); ++iClassifier)
	{
		IClassifier::SP pClassifier = *iClassifier;
		if (! pClassifier->OnStop() )
			Log::Warning( "ClassifierManager", "Failed to stop classifier %s.", pClassifier->GetName() );
	}
	m_Classifiers.clear();

	m_bActive = false;
	return true;
}

void ClassifierManager::OnClassifierOverride(IClassifier * a_pClassifier)
{
	if (m_bActive)
	{
		if (a_pClassifier->OnStop())
			a_pClassifier->SetState(IClassifier::AS_SUSPENDED);
	}
}

void ClassifierManager::OnClassifierOverrideEnd(IClassifier * a_pClassifier)
{
	if (m_bActive)
	{
		if (a_pClassifier->OnStart())
			a_pClassifier->SetState(IClassifier::AS_RUNNING);
	}
}

void ClassifierManager::OnSubscriber(const ITopics::SubInfo & a_Info)
{
	if (!a_Info.m_Subscribed)
	{
		for (ClassifierList::iterator iClassifier = m_Classifiers.begin(); 
			iClassifier != m_Classifiers.end();)
		{
			ProxyClassifier::SP spProxy = DynamicCast<ProxyClassifier>(*iClassifier++);
			if (!spProxy)
				continue;

			if (spProxy->GetOrigin() == a_Info.m_Origin)
			{
				Log::Status("ClassifierManager", "Removing proxy classifier %s for origin %s",
					spProxy->GetGUID().c_str(), a_Info.m_Origin.c_str());
				RemoveClassifier(spProxy);
			}
		}
	}
}

bool ClassifierManager::AddClassifier(const IClassifier::SP & a_spClassifier, bool a_bOverride)
{
	if (!a_spClassifier)
		return false;

	a_spClassifier->SetClassifierManager(this, a_bOverride);
	m_Classifiers.push_back(a_spClassifier);

	if (m_bActive && a_spClassifier->IsEnabled() )
	{
		if (a_spClassifier->OnStart())
			a_spClassifier->SetState(IClassifier::AS_RUNNING);
	}

	Log::Status("ClassifierManager", "Added classifier %s", a_spClassifier->GetRTTI().GetName().c_str());
	return true;
}

bool ClassifierManager::RemoveClassifier(const IClassifier::SP & a_spClassifier)
{
	for (ClassifierList::iterator iClassifier = m_Classifiers.begin();
		iClassifier != m_Classifiers.end(); ++iClassifier)
	{
		if ((*iClassifier) == a_spClassifier)
		{
			if ( a_spClassifier->IsActive())
				a_spClassifier->OnStop();

			a_spClassifier->SetClassifierManager(NULL, true);
			m_Classifiers.erase(iClassifier);
			return true;
		}
	}

	return false;
}

bool ClassifierManager::FindClassifiers(const std::string & a_Name, 
	std::vector<IClassifier::SP> & a_Overrides)
{
	for (ClassifierList::iterator iClassifier = m_Classifiers.begin();
		iClassifier != m_Classifiers.end(); ++iClassifier)
	{
		if ((*iClassifier)->GetName() == a_Name )
			a_Overrides.push_back((*iClassifier));
	}

	if (a_Overrides.size() > 0)
		return true;

	return false;
}

void ClassifierManager::OnClassifierEvent(const ITopics::Payload & a_Payload)
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
				if (ev == "add_classifier_proxy")
				{
					const std::string & classifierId = json["classifierId"].asString();
					const std::string & classifierName = json["name"].asString();

					bool bOverride = json["override"].asBool();

					Log::Status("ClassifierManager", "Adding proxy classifier %s, Id: %s, Override: %s",
						classifierName.c_str(), classifierId.c_str(), bOverride ? "True" : "False");

					ProxyClassifier::SP spProxy(new ProxyClassifier(classifierName, classifierId, bOverride, a_Payload.m_RemoteOrigin));
					AddClassifier(spProxy, bOverride);
				}
				else if (ev == "remove_classifier_proxy")
				{
					bool bSuccess = false;

					const std::string & classifierId = json["classifierId"].asString();
					for (ClassifierList::iterator iClassifier = m_Classifiers.begin();
						iClassifier != m_Classifiers.end(); ++iClassifier)
					{

						if ((*iClassifier)->GetGUID() == classifierId)
						{
							Log::Status("ClassifierManager", "Removing proxy classifier %s, Id: %s",
								(*iClassifier)->GetName(), classifierId.c_str());
							bSuccess = RemoveClassifier((*iClassifier));
							break;
						}
					}

					if (!bSuccess)
						Log::Warning("ClassifierManager", "Failed to remove proxy classifier %s", classifierId.c_str());
				}
				else if (ev == "error")
				{
					const std::string & failed_event = json["failed_event"].asString();
					Log::Error("ClassifierManager", "Received error on event: %s", failed_event.c_str());
				}
				else
				{
					Log::Warning("ClassifierManager", "Received unknown event: %s", json.toStyledString().c_str());
				}
			}
		}
	}
}


