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


#include "DepthExtractor.h"

#include "SelfInstance.h"
#include "sensors/SensorManager.h"
#include "sensors/ISensor.h"
#include "sensors/DepthVideoData.h"
#include "utils/Delegate.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/DepthImage.h"
#include "utils/ThreadPool.h"

#include "jsoncpp/json/json.h"

REG_SERIALIZABLE(DepthExtractor);
RTTI_IMPL(DepthExtractor, IExtractor);

const char * DepthExtractor::GetName() const
{
	return "ObjectExtractor";
}

bool DepthExtractor::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance != NULL)
	{
		pInstance->GetSensorManager()->RegisterForSensor("DepthVideoData",
			DELEGATE(DepthExtractor, OnAddSensor, ISensor *, this),
			DELEGATE(DepthExtractor, OnRemoveSensor, ISensor *, this));
	}

	Log::Status("ObjectExtractor", "ObjectExtractor started");
	return true;
}

bool DepthExtractor::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance != NULL)
		pInstance->GetSensorManager()->UnregisterForSensor("DepthVideoData", this);

	Log::Status("ObjectExtractor", "ObjectExtractor stopped");
	return true;
}

void DepthExtractor::OnAddSensor(ISensor * a_pSensor)
{
	Log::Status("ObjectExtractor", "Adding new depth video sensor %s", a_pSensor->GetSensorId().c_str());
	m_DepthVideoSensors.push_back(a_pSensor->shared_from_this());
	a_pSensor->Subscribe(DELEGATE(DepthExtractor, OnDepthVideoData, IData *, this));
}

void DepthExtractor::OnRemoveSensor(ISensor * a_pSensor)
{
	for (size_t i = 0; i<m_DepthVideoSensors.size(); ++i)
	{
		if (m_DepthVideoSensors[i].get() == a_pSensor)
		{
			m_DepthVideoSensors.erase(m_DepthVideoSensors.begin() + i);
			Log::Status("ObjectExtractor", "Removing depth video sensor %s", a_pSensor->GetSensorId().c_str());
			a_pSensor->Unsubscribe(this);
			break;
		}
	}
}

void DepthExtractor::OnDepthVideoData(IData * a_pData)
{
	DepthVideoData * pVideo = DynamicCast<DepthVideoData>(a_pData);
	if (pVideo != NULL)
	{
		DepthImage::SP spDepthImage(new DepthImage());
		spDepthImage->SetContent(pVideo->GetBinaryData());
		spDepthImage->SetOrigin(pVideo->GetOrigin()->GetSensorId());

		SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spDepthImage);
	}
}