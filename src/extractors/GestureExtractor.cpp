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

#include "GestureExtractor.h"

#include "SelfInstance.h"
#include "sensors/SensorManager.h"
#include "sensors/ISensor.h"
#include "sensors/GazeData.h"
#include "sensors/GestureData.h"
#include "utils/Delegate.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Gesture.h"
#include "utils/ThreadPool.h"


REG_SERIALIZABLE(GestureExtractor);
RTTI_IMPL( GestureExtractor, IExtractor );

const char * GestureExtractor::GetName() const
{
    return "GestureExtractor";
}

bool GestureExtractor::OnStart()
{
    // find all audio sensors, then subscribe to them to receive their input..
    SelfInstance::GetInstance()->GetSensorManager()->FindSensors(GazeData::GetStaticRTTI(), m_GazeSensors);
    for (SensorManager::SensorList::iterator iSensor = m_GazeSensors.begin(); iSensor != m_GazeSensors.end(); ++iSensor)
    {
        (*iSensor)->Subscribe( DELEGATE( GestureExtractor, OnGazeData, IData *, this ));
    }

    SelfInstance::GetInstance()->GetSensorManager()->FindSensors(GestureData::GetStaticRTTI(), m_GestureSensors);
    for (SensorManager::SensorList::iterator iSensor = m_GestureSensors.begin(); iSensor != m_GestureSensors.end(); ++iSensor)
    {
        (*iSensor)->Subscribe( DELEGATE( GestureExtractor, OnGestureData, IData *, this ) );
    }

    Log::Status("GestureExtractor", "GestureExtractor started");
    return true;
}

bool GestureExtractor::OnStop()
{
    for (SensorManager::SensorList::iterator iSensor = m_GazeSensors.begin(); iSensor != m_GazeSensors.end(); ++iSensor)
        (*iSensor)->Unsubscribe(this);
    m_GazeSensors.clear();

    Log::Status("GestureExtractor", "GestureExtractor stopped");
    return true;
}

void GestureExtractor::OnGazeData(IData * data)
{
    GazeData * pGaze = DynamicCast<GazeData>(data);
    if ( pGaze != NULL )
    {
        Gesture::SP spGesture( new Gesture("gaze") );
//        spImage->SetContent( std::string( (const char *)pVideo->GetBinaryData(), pVideo->GetBinaryLength() ) );

        SelfInstance::GetInstance()->GetBlackBoard()->AddThing( spGesture );
    }
}

void GestureExtractor::OnGestureData(IData * data)
{
    GestureData * gesture = DynamicCast<GestureData>(data);
    if( gesture != NULL )
    {
        Gesture::SP spGesture( new Gesture( gesture->GetGesture() ) );
        SelfInstance::GetInstance()->GetBlackBoard()->AddThing( spGesture );
    }
}