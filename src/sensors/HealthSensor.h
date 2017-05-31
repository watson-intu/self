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


#ifndef SELF_HW_DEVICE_H
#define SELF_HW_DEVICE_H

#include "SelfInstance.h"
#include "ISensor.h"
#include "HealthData.h"

#include "utils/TimerPool.h"
#include "utils/Time.h"

#include "SelfLib.h"

//! Base class for a Hardware Device sensor class
class SELF_API HealthSensor : public ISensor
{
public:
    RTTI_DECL();

    HealthSensor() : ISensor( "Health" ), m_fLowBatteryThreshold( 10.0f )
    {}

    //! ISerializable interface
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

    //! ISensor interface
    virtual const char * GetDataType()
    {
        return "HealthData";
    }

    virtual bool OnStart();
    virtual bool OnStop();
    virtual void OnPause();
    virtual void OnResume();

protected:
    //! Data
    std::vector<std::string>	m_SensorReadings;
    std::vector<std::string>	m_ErrorDiagnosis;
    int                         m_HealthSensorCheckInterval;
	float                       m_fLowBatteryThreshold;
};

#endif	// SELF_HW_DEVICE_H