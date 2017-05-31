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


#include "HealthSensor.h"

REG_SERIALIZABLE( HealthSensor );
RTTI_IMPL(HealthSensor, ISensor);

void HealthSensor::Serialize(Json::Value & json)
{
    ISensor::Serialize( json );
    json["m_HealthSensorCheckInterval"] = m_HealthSensorCheckInterval;
	json["m_fLowBatteryThreshold"] = m_fLowBatteryThreshold;
    SerializeVector( "m_SensorReadings", m_SensorReadings, json );
    SerializeVector( "m_ErrorDiagnosis", m_ErrorDiagnosis, json );
}

void HealthSensor::Deserialize(const Json::Value & json)
{
    ISensor::Deserialize( json );
    if ( json.isMember("m_HealthSensorCheckInterval") )
        m_HealthSensorCheckInterval = json["m_HealthSensorCheckInterval"].asInt();
	if ( json.isMember("m_fLowBatteryThreshold") )
		m_fLowBatteryThreshold = json["m_fLowBatteryThreshold"].asFloat();
    DeserializeVector( "m_SensorReadings", json, m_SensorReadings );
    DeserializeVector( "m_ErrorDiagnosis", json, m_ErrorDiagnosis );
}

bool HealthSensor::OnStart()
{
    Log::Debug( "HealthSensor", "OnStart() invoked." );
    return true;
}

bool HealthSensor::OnStop()
{
    Log::Debug( "HealthSensor", "OnStop() invoked." );
    return true;
}

void HealthSensor::OnPause()
{}

void HealthSensor::OnResume()
{}

