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


#include "PrivacyAgent.h"
#include "SelfInstance.h"
#include "services/ISpeechToText.h"
#include "sensors/VideoData.h"
#include "blackboard/Person.h"
#include "blackboard/ProxyIntent.h"
#include "blackboard/BlackBoard.h"

REG_SERIALIZABLE(PrivacyAgent);
RTTI_IMPL( PrivacyAgent, IAgent );

void PrivacyAgent::Serialize(Json::Value & json)
{
    IAgent::Serialize(json);
    json["m_LogSpeech"] = m_LogSpeech;
    json["m_LogCamera"] = m_LogCamera;
    json["m_EnableDynamicOptOut"] = m_EnableDynamicOptOut;
    json["m_bStoreAudio"] = m_bStoreAudio;
    json["m_MinimumAge"] = m_MinimumAge;
	json["m_CameraOffIntent"] = m_CameraOffIntent;
	json["m_CameraOnIntent"] = m_CameraOnIntent;
}

void PrivacyAgent::Deserialize(const Json::Value & json)
{
    IAgent::Deserialize(json);
    if( json.isMember("m_LogSpeech") )
        m_LogSpeech = json["m_LogSpeech"].asBool();
    if( json.isMember("m_LogCamera") )
        m_LogCamera = json["m_LogCamera"].asBool();
    if( json.isMember("m_EnableDynamicOptOut") )
        m_EnableDynamicOptOut = json["m_EnableDynamicOptOut"].asBool();
    if( json.isMember("m_bStoreAudio") )
        m_bStoreAudio = json["m_bStoreAudio"].asBool();
    if( json.isMember("m_MinimumAge") )
        m_MinimumAge = json["m_MinimumAge"].asInt();
	if (json.isMember("m_CameraOffIntent"))
		m_CameraOffIntent = json["m_CameraOffIntent"].asString();
	if (json.isMember("m_CameraOnIntent"))
		m_CameraOnIntent = json["m_CameraOnIntent"].asString();
}

bool PrivacyAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;
	
	pInstance->GetSensorManager()->RegisterForSensor("VideoData",
		DELEGATE(PrivacyAgent, OnAddVideoSensor, ISensor *, this),
		DELEGATE(PrivacyAgent, OnRemoveVideoSensor, ISensor *, this));
	
	ISpeechToText * pSTT = Config::Instance()->FindService<ISpeechToText>();
	if( pSTT != NULL && !m_LogSpeech)
		pSTT->SetLearningOptOut(true);

    if(!m_LogCamera)
		SetCameraActivity(false);

    pInstance->GetBlackBoard()->SubscribeToType( "Person",
		DELEGATE( PrivacyAgent, OnPerson, const ThingEvent &, this ), TE_ADDED);
	pInstance->GetBlackBoard()->SubscribeToType( "ProxyIntent",
		DELEGATE(PrivacyAgent, OnProxyIntent, const ThingEvent &, this), TE_ADDED);
    return true;
}

bool PrivacyAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert(pBlackboard != NULL);

	pBlackboard->UnsubscribeFromType("Person", this);
	pBlackboard->UnsubscribeFromType("ProxyIntent", this);
	m_ActiveCameras.clear();
    return true;
}

void PrivacyAgent::OnAddVideoSensor(ISensor * a_pSensor)
{
	bool found = false;
	for (size_t i = 0; i < m_ActiveCameras.size(); ++i)
	{
		if (m_ActiveCameras[i].get() == a_pSensor)
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		m_ActiveCameras.push_back(a_pSensor->shared_from_this());
	}
}

void PrivacyAgent::OnRemoveVideoSensor(ISensor * a_pSensor)
{
	for (size_t i = 0; i < m_ActiveCameras.size(); ++i)
	{
		if (m_ActiveCameras[i].get() == a_pSensor)
		{
			m_ActiveCameras.erase(m_ActiveCameras.begin() + i);			
			break;
		}
	}
}

void PrivacyAgent::OnProxyIntent(const ThingEvent & a_ThingEvent)
{
	ProxyIntent::SP spProxyIntent = DynamicCast<ProxyIntent>( a_ThingEvent.GetIThing() );
	if (spProxyIntent)
	{
		Json::Value root = spProxyIntent->GetIntent();
		if (root.isMember("conversation"))
		{		
			std::string intent = JsonHelpers::Resolve(root, "conversation/intents/0/intent").asString();
			if (m_CameraOffIntent.compare(intent) == 0) {
				SetCameraActivity(false);
				AddPrivacyThing(a_ThingEvent);
				Log::Debug("PrivacyAgent", "Turning off Camera!!");
			}
			else if (m_CameraOnIntent.compare(intent) == 0)
			{
				SetCameraActivity(true);
				AddPrivacyThing(a_ThingEvent);
				Log::Debug("PrivacyAgent", "Turning On Camera!!");
			}
			else
			{
				Log::Debug("PrivacyAgent", "Not the correct Proxy Intent, not doing anything...");
			}
		}
	}
}

void PrivacyAgent::OnPerson(const ThingEvent & a_ThingEvent)
{
	Person::SP spPerson = DynamicCast<Person>( a_ThingEvent.GetIThing() );
	if(spPerson && m_EnableDynamicOptOut)
	{
		std::string age = spPerson->GetAgeRange();
		int lowerAge = atoi(StringUtil::LeftTrim(age, "-").c_str());

		ISpeechToText * pSTT = Config::Instance()->FindService<ISpeechToText>();
		if(lowerAge > m_MinimumAge && !m_bStoreAudio)
		{
			m_bStoreAudio = true;
			if ( pSTT != NULL )
				pSTT->SetLearningOptOut(false);
			m_spAgeTimeout = TimerPool::Instance()->StartTimer(VOID_DELEGATE(PrivacyAgent, OnAgeTimeout, this), m_AgeTimeout, true, false);
		}
		else if(lowerAge < m_MinimumAge && m_bStoreAudio)
		{
			m_bStoreAudio = false;
			if ( pSTT != NULL )
				pSTT->SetLearningOptOut(true);
            AddPrivacyThing(a_ThingEvent);
		}
	}
}

void PrivacyAgent::SetCameraActivity(bool isActive)
{
	for (size_t i = 0; i < m_ActiveCameras.size(); ++i)
	{
		if (!isActive)
		{
			if(!m_ActiveCameras[i].get()->IsPaused())
				m_ActiveCameras[i].get()->OnPause();
		}			
		else
		{
			if(m_ActiveCameras[i].get()->IsPaused())
				m_ActiveCameras[i].get()->OnResume();
		}		
	}
}

void PrivacyAgent::OnAgeTimeout()
{
    if(m_bStoreAudio)
    {
		ISpeechToText * pSTT = Config::Instance()->FindService<ISpeechToText>();
		if ( pSTT != NULL )
			pSTT->SetLearningOptOut(false);
        m_bStoreAudio = false;
    }
}

void PrivacyAgent::AddPrivacyThing(const ThingEvent & a_ThingEvent)
{
	a_ThingEvent.GetIThing()->AddChild( IThing::SP(new IThing( TT_COGNITIVE, "Privacy" )) );
}


