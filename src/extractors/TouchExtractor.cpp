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


#include "TouchExtractor.h"

#include "SelfInstance.h"
#include "sensors/SensorManager.h"
#include "sensors/ISensor.h"
#include "sensors/TouchData.h"
#include "utils/Delegate.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Touch.h"
#include "utils/ThreadPool.h"

#include "jsoncpp/json/json.h"

REG_SERIALIZABLE(TouchExtractor);
RTTI_IMPL(TouchExtractor, IExtractor);

const char * TouchExtractor::GetName() const
{
	return "TouchExtractor";
}

bool TouchExtractor::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance != NULL)
	{
		pInstance->GetSensorManager()->RegisterForSensor("TouchData",
			DELEGATE(TouchExtractor, OnAddSensor, ISensor *, this),
			DELEGATE(TouchExtractor, OnRemoveSensor, ISensor *, this));
	}

	Log::Status("TouchExtractor", "TouchExtractor started");
	return true;
}

bool TouchExtractor::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance != NULL)
		pInstance->GetSensorManager()->UnregisterForSensor("TouchData", this);

	Log::Status("TouchExtractor", "TouchExtractor stopped");
	return true;
}

void TouchExtractor::OnAddSensor(ISensor * a_pSensor)
{
	Log::Status("TouchExtractor", "Adding new touch sensor %s", a_pSensor->GetSensorId().c_str());
	m_TouchSensors.push_back(a_pSensor->shared_from_this());
	a_pSensor->Subscribe(DELEGATE(TouchExtractor, OnTouchData, IData *, this));
}

void TouchExtractor::OnRemoveSensor(ISensor * a_pSensor)
{
	for (size_t i = 0; i<m_TouchSensors.size(); ++i)
	{
		if (m_TouchSensors[i].get() == a_pSensor)
		{
			m_TouchSensors.erase(m_TouchSensors.begin() + i);
			Log::Status("TouchExtractor", "Removing touch sensor %s", a_pSensor->GetSensorId().c_str());
			a_pSensor->Unsubscribe(this);
			break;
		}
	}
}

void TouchExtractor::OnTouchData(IData * a_pData)
{
	TouchData * pTouch = DynamicCast<TouchData>(a_pData);
	if (pTouch != NULL)
	{
		Touch::SP spTouch(new Touch());
		spTouch->SetEngage(pTouch->GetEngaged());
		spTouch->SetSensorName(pTouch->GetSensorName());

		SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spTouch);
	}
}