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


#include "SleepAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Health.h"
#include "utils/Time.h"

REG_SERIALIZABLE(SleepAgent);
RTTI_IMPL(SleepAgent, IAgent);

void SleepAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize( json );

	json["m_WakeTime"] = m_WakeTime;
	json["m_SleepTime"] = m_SleepTime;
	SerializeVector("m_HealthSensorMasks", m_HealthSensorMasks, json);
}

void SleepAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize( json );

	if (json.isMember("m_WakeTime"))
		m_WakeTime = json["m_WakeTime"].asDouble();
	if (json.isMember("m_SleepTime"))
		m_SleepTime = json["m_SleepTime"].asDouble();

	DeserializeVector("m_HealthSensorMasks", json, m_HealthSensorMasks);
	if (m_HealthSensorMasks.size() == 0)
		m_HealthSensorMasks.push_back("Diagnosis/Temperature/*");
}

bool SleepAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->SubscribeToType("Health",
		DELEGATE(SleepAgent, OnHealth, const ThingEvent &, this));

	OnWake();
	return true;
}

bool SleepAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->UnsubscribeFromType("Health", this);
	m_spWakeTimer.reset();
	m_spSleepTimer.reset();

	return true;
}

void SleepAgent::OnHealth(const ThingEvent & a_ThingEvent)
{
	Health::SP spHealth = DynamicCast<Health>(a_ThingEvent.GetIThing());
	if (spHealth->IsError() && spHealth->GetState() == "CRITICAL")
	{
		bool bMatched = false;
		for (size_t i = 0; i < m_HealthSensorMasks.size() && !bMatched; ++i)
			if (StringUtil::WildMatch(m_HealthSensorMasks[i].c_str(), spHealth->GetHealthName().c_str()))
				bMatched = true;

		if (bMatched && !m_bSleeping)
		{
			Log::Debug("SleepAgent", "Health error is triggering rest state: %s",
				spHealth->ToJson().toStyledString().c_str());

			m_bSleeping = true;

			m_spWakeTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(SleepAgent, OnWake, this), m_SleepTime, true, false);
			SelfInstance::GetInstance()->GetBlackBoard()->AddThing(Health::SP(new Health("Sleep", "Rest", "", (float)m_SleepTime, false, true)));
			Log::Status("Health", "Entering Rest state for %g seconds.", m_SleepTime);
		}
	}
}

void SleepAgent::OnSleep()
{
	m_bSleeping = true;

	m_spWakeTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(SleepAgent, OnWake, this), m_SleepTime, true, false);
	SelfInstance::GetInstance()->GetBlackBoard()->AddThing(Health::SP(new Health("Sleep", "Sleep", "", (float)m_SleepTime, false, true)));
	Log::Status("Health", "Entering Sleep state for %g seconds.", m_SleepTime);
}

void SleepAgent::OnWake()
{
	m_bSleeping = false;

	m_spSleepTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(SleepAgent, OnSleep, this), m_WakeTime, true, false);
	SelfInstance::GetInstance()->GetBlackBoard()->AddThing(Health::SP(new Health("Sleep", "Wake", "", (float)m_WakeTime, false, true)));
	Log::Status("Health", "Entering Wake state for %g seconds.", m_WakeTime);
}

