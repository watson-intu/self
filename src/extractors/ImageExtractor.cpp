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


#include "ImageExtractor.h"

#include "SelfInstance.h"
#include "sensors/SensorManager.h"
#include "sensors/ISensor.h"
#include "sensors/VideoData.h"
#include "utils/Delegate.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Image.h"
#include "utils/ThreadPool.h"

#include "jsoncpp/json/json.h"

REG_SERIALIZABLE(ImageExtractor);
RTTI_IMPL( ImageExtractor, IExtractor );

const char * ImageExtractor::GetName() const
{
    return "ImageExtractor";
}

bool ImageExtractor::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance != NULL )
	{
		pInstance->GetSensorManager()->RegisterForSensor( "VideoData", 
			DELEGATE( ImageExtractor, OnAddSensor, ISensor *, this ),
			DELEGATE( ImageExtractor, OnRemoveSensor, ISensor *, this ) );
	}

    Log::Status("ImageExtractor", "ImageExtractor started");
	return true;
}

bool ImageExtractor::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance != NULL )
		pInstance->GetSensorManager()->UnregisterForSensor( "VideoData", this );

    Log::Status("ImageExtractor", "ImageExtractor stopped");
	return true;
}

void ImageExtractor::OnAddSensor( ISensor * a_pSensor )
{
	Log::Status( "ImageExtractor", "Adding new video sensor %s", a_pSensor->GetSensorId().c_str() );
	m_VideoSensors.push_back( a_pSensor->shared_from_this() );
	a_pSensor->Subscribe( DELEGATE( ImageExtractor, OnVideoData, IData *, this ) );
}

void ImageExtractor::OnRemoveSensor( ISensor * a_pSensor )
{
	for(size_t i=0;i<m_VideoSensors.size();++i)
	{
		if ( m_VideoSensors[i].get() == a_pSensor )
		{
			m_VideoSensors.erase( m_VideoSensors.begin() + i );
			Log::Status( "ImageExtractor", "Removing video sensor %s", a_pSensor->GetSensorId().c_str() );
			a_pSensor->Unsubscribe( this );
			break;
		}
	}
}

void ImageExtractor::OnVideoData(IData * a_pData)
{
    VideoData * pVideo = DynamicCast<VideoData>(a_pData);
	if ( pVideo != NULL )
	{
		Image::SP spImage( new Image() );
		spImage->SetContent( pVideo->GetBinaryData() );
		spImage->SetOrigin( pVideo->GetOrigin()->GetSensorId() );

		SelfInstance::GetInstance()->GetBlackBoard()->AddThing( spImage );
	}
}