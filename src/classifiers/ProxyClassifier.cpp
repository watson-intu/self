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


#include "ProxyClassifier.h"
#include "SelfInstance.h"
#include "ClassifierManager.h"
#include "utils/UniqueID.h"
#include "topics/ITopics.h"

RTTI_IMPL(ProxyClassifier, IClassifier);
REG_SERIALIZABLE(ProxyClassifier);

ProxyClassifier::ProxyClassifier(const std::string & a_ClassifierName,
	const std::string & a_InstanceId, bool a_bOverride, const std::string & a_Origin) :
	m_ClassifierName(a_ClassifierName),
	m_Origin(a_Origin),
	m_bOverride(a_bOverride)
{
	SetGUID( a_InstanceId );
}

ProxyClassifier::ProxyClassifier() :
	m_bOverride(false)
{}

ProxyClassifier::~ProxyClassifier()
{}

void ProxyClassifier::Serialize(Json::Value & json)
{
	IClassifier::Serialize(json);

	json["m_ClassifierName"] = m_ClassifierName;
	json["m_Origin"] = m_Origin;
	json["m_bOverride"] = m_bOverride;
}

void ProxyClassifier::Deserialize(const Json::Value & json)
{
	IClassifier::Deserialize(json);

	m_ClassifierName = json["m_ClassifierName"].asString();
	m_Origin = json["m_Origin"].asString();
	m_bOverride = json["m_bOverride"].asBool();
}

const char * ProxyClassifier::GetName() const
{
	return m_ClassifierName.c_str();
}

bool ProxyClassifier::OnStart()
{
	SendEvent("start_classifier");
	return true;
}

bool ProxyClassifier::OnStop()
{
	SendEvent("stop_classifier");
	return true;
}

void ProxyClassifier::SendEvent(const std::string & a_EventName)
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert(pInstance != NULL);
	ITopics * pTopics = pInstance->GetTopics();
	assert(pTopics != NULL);

	Json::Value ev;
	ev["event"] = a_EventName;
	ev["classifierId"] = GetGUID();

	pTopics->Send(m_Origin, "classifier-manager", ev.toStyledString());
}
