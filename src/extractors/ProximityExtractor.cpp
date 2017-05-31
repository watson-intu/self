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


#include "ProximityExtractor.h"

#include "SelfInstance.h"
#include "sensors/SensorManager.h"
#include "sensors/ISensor.h"
#include "sensors/SonarData.h"
#include "sensors/LaserData.h"
#include "utils/Delegate.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Proximity.h"
#include "utils/ThreadPool.h"

#include "jsoncpp/json/json.h"

REG_SERIALIZABLE(ProximityExtractor);
RTTI_IMPL( ProximityExtractor, IExtractor );


const char * ProximityExtractor::GetName() const
{
    return "ProximityExtractor";
}

void ProximityExtractor::Deserialize(const Json::Value & json )
{
	IExtractor::Deserialize( json );

    if ( json.isMember("m_MinSonarValue") )
        m_MinSonarValue = json["m_MinSonarValue"].asFloat();
    if ( json.isMember("m_MaxSonarValue") )
        m_MaxSonarValue = json["m_MaxSonarValue"].asFloat();
    if ( json.isMember("m_SamplesToAverage") )
        m_SamplesToAverage = json["m_SamplesToAverage"].asInt();
}

void ProximityExtractor::Serialize( Json::Value & json )
{
	IExtractor::Serialize( json );

    json["m_MinSonarValue"] = m_MinSonarValue;
    json["m_MaxSonarValue"] = m_MaxSonarValue;
    json["m_SamplesToAverage"] = m_SamplesToAverage;
}

bool ProximityExtractor::OnStart()
{
    // find all audio sensors, then subscribe to them to receive their input..
    SelfInstance::GetInstance()->GetSensorManager()->FindSensors(SonarData::GetStaticRTTI(), m_SonarSensors);
    for (SensorManager::SensorList::iterator iSensor = m_SonarSensors.begin(); iSensor != m_SonarSensors.end(); ++iSensor)
    {
        (*iSensor)->Subscribe( DELEGATE( ProximityExtractor, OnSonarData, IData *, this ));
        Log::Status("ProximityExtractor", "Sonar Sensor subscription started");
    }

    SelfInstance::GetInstance()->GetSensorManager()->FindSensors(LaserData::GetStaticRTTI(), m_LaserSensors);
    for (SensorManager::SensorList::iterator iSensor = m_LaserSensors.begin(); iSensor != m_LaserSensors.end(); ++iSensor)
    {
        (*iSensor)->Subscribe( DELEGATE( ProximityExtractor, OnLaserData, IData *, this  ));
        Log::Status("ProximityExtractor", "Laser Sensor subscription started");
    }
    Log::Status("ProximityExtractor", "ProximityExtractor started");
	return true;
}

bool ProximityExtractor::OnStop()
{
    for (SensorManager::SensorList::iterator iSensor = m_SonarSensors.begin(); iSensor != m_SonarSensors.end(); ++iSensor)
        (*iSensor)->Unsubscribe(this);
    m_SonarSensors.clear();

    Log::Status("ProximityExtractor", "ProximityExtractor stopped");
	return true;
}

void ProximityExtractor::OnSonarData(IData * data)
{
    SonarData* pSonarData = DynamicCast<SonarData>(data);
	if ( pSonarData != NULL )
	{
        m_CurrentAverage -= m_CurrentAverage / (float)m_SamplesToAverage;
        m_CurrentAverage += pSonarData->GetDistance() / (float)m_SamplesToAverage;
		if ( m_MinSonarValue < m_CurrentAverage && m_CurrentAverage < m_MaxSonarValue )
        {
            Log::Debug("ProximityExtractor", "In range %.2f to %.2f : %.2f", m_MinSonarValue, m_MaxSonarValue, m_CurrentAverage);
            Proximity::SP spProximity( new Proximity("Sonar", true, pSonarData->GetDistance() ) );
		    SelfInstance::GetInstance()->GetBlackBoard()->AddThing( spProximity );
        }
    }
}

void ProximityExtractor::OnLaserData(IData * data)
{
    LaserData* pLaserData = DynamicCast<LaserData>(data);
    if(pLaserData != NULL)
    {
        Log::Debug("ProximityExtractor", "Adding proximity data");
        Proximity::SP spProximity( new Proximity("Laser", true, pLaserData->GetDistance() ) );
        SelfInstance::GetInstance()->GetBlackBoard()->AddThing( spProximity );
    }
}