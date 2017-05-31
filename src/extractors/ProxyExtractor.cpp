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


#include "ProxyExtractor.h"
#include "SelfInstance.h"
#include "FeatureManager.h"
#include "utils/UniqueID.h"
#include "topics/ITopics.h"

RTTI_IMPL(ProxyExtractor, IExtractor);
REG_SERIALIZABLE(ProxyExtractor);

ProxyExtractor::ProxyExtractor(const std::string & a_ExtractorName, const std::string & a_InstanceId, bool a_bOverride, const std::string & a_Origin) :
	m_ExtractorName(a_ExtractorName),
	m_Origin(a_Origin),
	m_bOverride(a_bOverride)
{
	SetGUID( a_InstanceId );
}

ProxyExtractor::ProxyExtractor() :
	m_bOverride(false)
{}

ProxyExtractor::~ProxyExtractor()
{}

void ProxyExtractor::Serialize(Json::Value & json)
{
	IExtractor::Serialize(json);
	json["m_Origin"] = m_Origin;
	json["m_ExtractorName"] = m_ExtractorName;
	json["m_bOverride"] = m_bOverride;
}

void ProxyExtractor::Deserialize(const Json::Value & json)
{
	IExtractor::Deserialize(json);
	m_Origin = json["m_Origin"].asString();
	m_ExtractorName = json["m_ExtractorName"].asString();
	m_bOverride = json["m_bOverride"].asBool();
}

bool ProxyExtractor::OnStart()
{
	SendEvent("start_extractor");
	return true;
}

bool ProxyExtractor::OnStop()
{
	SendEvent("stop_extractor");
	return true;
}

const char * ProxyExtractor::GetName() const
{
	return m_ExtractorName.c_str();
}

void ProxyExtractor::SendEvent(const std::string & a_EventName)
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert(pInstance != NULL);
	ITopics * pTopics = pInstance->GetTopics();
	assert(pTopics != NULL);

	Json::Value ev;
	ev["event"] = a_EventName;
	ev["extractorId"] = GetGUID();

	pTopics->Send(m_Origin, "feature-manager", ev.toStyledString());
}
