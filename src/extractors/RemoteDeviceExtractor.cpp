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
#include "blackboard/BlackBoard.h"
#include "RemoteDeviceExtractor.h"

REG_SERIALIZABLE(RemoteDeviceExtractor);
RTTI_IMPL( RemoteDeviceExtractor, IExtractor );

const char * RemoteDeviceExtractor::GetName() const
{
    return "RemoteDeviceExtractor";
}

bool RemoteDeviceExtractor::OnStart()
{
    // find all remote device sensors, then subscribe to them to receive their input..
    SelfInstance::GetInstance()->GetSensorManager()->FindSensors(RemoteDeviceData::GetStaticRTTI(), m_RemoteDeviceSensors);
    for (SensorManager::SensorList::iterator iSensor = m_RemoteDeviceSensors.begin(); iSensor != m_RemoteDeviceSensors.end(); ++iSensor)
    {
        (*iSensor)->Subscribe( DELEGATE( RemoteDeviceExtractor, OnRemoteDeviceData, IData *, this ));
    }

    Log::Status("RemoteDeviceExtractor", "RemoteDeviceExtractor started");
    return true;
}

bool RemoteDeviceExtractor::OnStop()
{
    for (SensorManager::SensorList::iterator iSensor = m_RemoteDeviceSensors.begin(); iSensor != m_RemoteDeviceSensors.end(); ++iSensor)
        (*iSensor)->Unsubscribe(this);
    m_RemoteDeviceSensors.clear();

    Log::Status("RemoteDeviceExtractor", "RemoteDeviceExtractor stopped");
    return true;
}

void RemoteDeviceExtractor::OnRemoteDeviceData(IData * data)
{
    // TODO: Needs to make sense if it is an image, text, or environment - defaulting to environment now
    RemoteDeviceData * pRemoteDevice = DynamicCast<RemoteDeviceData>(data);
    if ( pRemoteDevice != NULL )
    {
        Environment::SP spEnvironment( new Environment() );
        if ( spEnvironment->Create( pRemoteDevice->GetContent() ) )
	        SelfInstance::GetInstance()->GetBlackBoard()->AddThing( spEnvironment );
    }
}

