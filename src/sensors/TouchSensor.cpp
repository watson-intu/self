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


#include "TouchSensor.h"

RTTI_IMPL_EMBEDDED(TouchSensor, TouchTranslation, ISerializable);

void TouchSensor::TouchTranslation::Serialize(Json::Value & json)
{
	SerializeVector("m_Conditions", m_Conditions, json);
	json["m_TouchType"] = m_TouchType;
}

void TouchSensor::TouchTranslation::Deserialize(const Json::Value & json)
{
	DeserializeVector("m_Conditions", json, m_Conditions);
	m_TouchType = json["m_TouchType"].asString();
}


REG_SERIALIZABLE(TouchSensor);
RTTI_IMPL(TouchSensor, ISensor);

void TouchSensor::Serialize(Json::Value & json)
{
	ISensor::Serialize(json);

	SerializeVector("m_TouchTranslations", m_TouchTranslations, json);
}

void TouchSensor::Deserialize(const Json::Value & json)
{
	ISensor::Deserialize(json);
	DeserializeVector("m_TouchTranslations", json, m_TouchTranslations);
}

bool TouchSensor::OnStart()
{
	Log::Debug("TouchSensor", "OnStart() invoked.");
	return true;
}

bool TouchSensor::OnStop()
{
	Log::Debug("TouchSensor", "OnStop() invoked.");
	return true;
}

void TouchSensor::OnPause()
{}

void TouchSensor::OnResume()
{}

