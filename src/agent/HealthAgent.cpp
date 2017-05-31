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


#include "SelfInstance.h"
#include "HealthAgent.h"
#include "sensors/HealthData.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Goal.h"
#include "blackboard/Health.h"
#include "utils/IService.h"
#include "utils/TimerPool.h"

REG_SERIALIZABLE(HealthAgent);
RTTI_IMPL(HealthAgent, IAgent);

void HealthAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
	json["m_HealthCheckInterval"] = m_HealthCheckInterval;
	json["m_NetworkCongestionThreshold"] = m_NetworkCongestionThreshold;
	SerializeVector("m_HealthCheckServices", m_HealthCheckServices, json);
}

void HealthAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
	if (json.isMember("m_HealthCheckInterval"))
		m_HealthCheckInterval = json["m_HealthCheckInterval"].asInt();
	if (json.isMember("m_NetworkCongestionThreshold"))
		m_NetworkCongestionThreshold = json["m_NetworkCongestionThreshold"].asInt();
	DeserializeVector("m_HealthCheckServices", json, m_HealthCheckServices);
}

bool HealthAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert(pBlackboard != NULL);

	// subscribe to Health objects which then create goals if triggerGoal is set to true
	pBlackboard->SubscribeToType("Health",
		DELEGATE(HealthAgent, OnHealth, const ThingEvent &, this), TE_ADDED);

	// subscribe to every sensor that outputs HealthData
	pInstance->GetSensorManager()->RegisterForSensor("HealthData",
		DELEGATE(HealthAgent, OnAddHealthSensor, ISensor *, this),
		DELEGATE(HealthAgent, OnRemoveHealthSensor, ISensor *, this));

	// first service check
	OnCheckServiceStatus();

	// start timer to periodically check for service statuses
	m_HealthCheckTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(HealthAgent, OnCheckServiceStatus, this),
		m_HealthCheckInterval, true, true);
	return true;
}

bool HealthAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->UnsubscribeFromType("Health", this);
	for (SensorManager::SensorList::iterator iSensor = m_HealthSensors.begin(); iSensor != m_HealthSensors.end(); ++iSensor)
		(*iSensor)->Unsubscribe(this);

	m_HealthCheckTimer.reset();
	return true;
}

void HealthAgent::OnHealth(const ThingEvent & a_ThingEvent)
{
	Health::SP spHealth = DynamicCast<Health>(a_ThingEvent.GetIThing());

	if (spHealth->IsTriggerGoal())
	{
		Goal::SP spGoal(new Goal());
		if (!spHealth->GetErrorName().empty())
			spGoal->SetName(spHealth->GetErrorName());
		else
			spGoal->SetName(spHealth->GetHealthName());

		spGoal->GetParams()["mac"] = SelfInstance::GetInstance()->GetLocalConfig().m_MacId;
		spGoal->GetParams()["name"] = SelfInstance::GetInstance()->GetLocalConfig().m_Name;
		if (!spHealth->GetErrorName().empty())
			spGoal->GetParams()["sensor"] = spHealth->GetHealthName();
		spGoal->GetParams()["value"] = spHealth->GetHealthValue();

		SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spGoal);
		Log::Status("HealthAgent", "Creating a goal for %s", spGoal->GetName().c_str());
	}
}

void HealthAgent::OnAddHealthSensor(ISensor * a_pSensor)
{
	Log::Status("HealthAgent", "Adding health sensor %s", a_pSensor->GetSensorName().c_str());
	m_HealthSensors.push_back(a_pSensor->shared_from_this());
	a_pSensor->Subscribe(DELEGATE(HealthAgent, OnHealthSensorData, IData *, this));
}

void HealthAgent::OnRemoveHealthSensor(ISensor * a_pSensor)
{
	Log::Status("HealthAgent", "Removing health sensor %s", a_pSensor->GetSensorName().c_str());

	a_pSensor->Unsubscribe(this);
	for (size_t i = 0; i < m_HealthSensors.size(); ++i)
		if (m_HealthSensors[i].get() == a_pSensor)
		{
			m_HealthSensors.erase(m_HealthSensors.begin() + i);
			break;
		}
}

// process HealthData output from sensors (hardware)
void HealthAgent::OnHealthSensorData(IData * data)
{
	HealthData * pHealthData = DynamicCast<HealthData>(data);
	if (pHealthData != NULL)
	{
		std::string healthName = pHealthData->GetHealthName();
		const Json::Value & params = pHealthData->GetContent();

		bool triggerGoal = false;
		if (healthName == "RemoteNetwork" && params["error"] == true)
		{
			healthName = "NetworkFailure";
			triggerGoal = true;
		}

		if (params["error"] == true && params["raiseAlert"] == true)
		{
			if (params["state"] == "CRITICAL")
				triggerGoal = true;
		}

		if (params.isMember("sensor"))
		{
			std::string sensor = params["sensor"].asCString();
			Health::SP spHealth(new Health("Sensors", sensor, params["state"].asCString(),
				params["value"].asFloat(), params["error"].asBool(), triggerGoal));
			spHealth->SetErrorName(healthName);
			SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spHealth);
			Log::Debug("HealthAgent", "Health Object: %s, %s, %d", sensor.c_str(), params.toStyledString().c_str(), triggerGoal);
		}
		else
		{
			SelfInstance::GetInstance()->GetBlackBoard()->AddThing(
				Health::SP(new Health("Sensors", healthName, params["state"].asCString(),
					params["value"].asFloat(), params["error"].asBool(), triggerGoal)));
			Log::Debug("HealthAgent", "Health Object: %s, %s, %d", healthName.c_str(), params.toStyledString().c_str(), triggerGoal);
		}

	}
}

// check services for health status
void HealthAgent::OnCheckServiceStatus()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert(pInstance != NULL);

	ServiceList services = pInstance->GetServiceList();

	m_FailedServices = 0;
	m_WorkingServices = 0;
	for (ServiceList::const_iterator iService = services.begin(); iService != services.end(); ++iService)
	{
		IService *pService = (*iService).get();
		if (pService == NULL)
			continue;
		// check if service is enabled
		if (!pService->IsEnabled())
			continue;
		if (std::find(m_HealthCheckServices.begin(), m_HealthCheckServices.end(), pService->GetServiceId()) == m_HealthCheckServices.end())
			continue;

		if (!pService->IsConfigured())
		{
			pInstance->GetBlackBoard()->AddThing(Health::SP(new Health("Configuration", pService->GetServiceId(), "NOT_CONFIGURED", false)));
			continue;
		}

		// get service status and put Health object on blackboard
		Log::DebugHigh("HealthAgent", "Checking status of service %s.", pService->GetRTTI().GetName().c_str());
		pService->GetServiceStatus(DELEGATE(HealthAgent, OnGetServiceStatus, const IService::ServiceStatus &, this));
	}

	int currentTimeoutCount = IService::sm_Timeouts.load();
	if (currentTimeoutCount - m_LastTimeoutCount >= m_NetworkCongestionThreshold)
	{
		//	add network congestion health object to blackboard
		Log::DebugHigh("Health", "Timeouts: (current %d - last %d) exceeds threshold %d so adding timeout health object",
			currentTimeoutCount, m_LastTimeoutCount, m_NetworkCongestionThreshold);

		pInstance->GetBlackBoard()->AddThing(Health::SP(
			new Health("Network", "Timeouts", "", (float)currentTimeoutCount, false, false)));
		m_LastTimeoutCount = currentTimeoutCount;
	}
}

void HealthAgent::OnGetServiceStatus(const IService::ServiceStatus & a_Status)
{
	Log::Debug("HealthAgent", "Status of service: %s is %s", a_Status.m_ServiceId.c_str(), a_Status.m_Status ? "UP" : "DOWN");

	// add health object to blackboard
	SelfInstance::GetInstance()->GetBlackBoard()->AddThing(Health::SP(new Health("Services", a_Status.m_ServiceId, !a_Status.m_Status, false)));

	std::map<std::string, bool>::iterator it = m_HealthStatusMap.find(a_Status.m_ServiceId);
	if (it != m_HealthStatusMap.end())
	{
		// update the map if the service status changed
		if (a_Status.m_Status != it->second)
		{
			m_HealthStatusMap[a_Status.m_ServiceId] = a_Status.m_Status;
			// if the service status is down, create a failed plan
			if (!a_Status.m_Status)
			{
				Log::Error("Health", "%s service is down!", a_Status.m_ServiceId.c_str());
				m_FailedServices++;
			}
			else
				m_WorkingServices++;
		}

	}
	else // no health status found for this service
	{
		m_HealthStatusMap[a_Status.m_ServiceId] = a_Status.m_Status;
		// if the service status is down, create a failed plan
		if (!a_Status.m_Status)
		{
			Log::Error("Health", "%s service is down!", a_Status.m_ServiceId.c_str());
			m_FailedServices++;
		}
		else
			m_WorkingServices++;
	}

	// this condition ensures that all services have returned with a status
	if (m_WorkingServices + m_FailedServices == m_HealthCheckServices.size())
	{
		Log::Debug("HealthAgent", "Services with new status: %u working, %u failed, %d check-enabled",
			m_WorkingServices.load(), m_FailedServices.load(), m_HealthCheckServices.size());

		// if there is at least one service failure, create a service failure goal..
		if (m_FailedServices > 0 && !m_bInServiceFailureState)
		{
			Log::Error("Health", "%u out of %d health check enabled services failed. Creating a ServiceFailure goal...",
				m_FailedServices.load(), m_HealthCheckServices.size());

			SelfInstance::GetInstance()->GetBlackBoard()->AddThing(Health::SP(new Health("Services", "ServiceFailure", true, true)));
			m_bInServiceFailureState = true;
		}
		// if all services are up and we were in a service failure state previously, create a service recovery goal..
		else if (m_FailedServices == 0 && m_bInServiceFailureState)
		{
			Log::Status("Health", "All services are up. Creating a ServiceRecovery goal");
			SelfInstance::GetInstance()->GetBlackBoard()->AddThing(Health::SP(new Health("Services", "ServiceRecovery", false, true)));
			m_bInServiceFailureState = false;
		}
	}
}
