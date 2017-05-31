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


#include "DepthCamera.h"

REG_SERIALIZABLE( DepthCamera );
RTTI_IMPL(DepthCamera, ISensor);

void DepthCamera::Serialize(Json::Value & json)
{
    ISensor::Serialize( json );
    json["m_fFramesPerSec"] = m_fFramesPerSec;
}

void DepthCamera::Deserialize(const Json::Value & json)
{
    ISensor::Deserialize( json );

    if ( json.isMember( "m_fFramesPerSec" ) )
        m_fFramesPerSec = json["m_fFramesPerSec"].asFloat();
}

bool DepthCamera::OnStart()
{
    Log::Debug( "DepthCamera", "OnStart() invoked." );
    return true;
}

bool DepthCamera::OnStop()
{
    Log::Debug( "DepthCamera", "OnStop() invoked." );
    return true;
}

void DepthCamera::OnPause()
{}

void DepthCamera::OnResume()
{}
