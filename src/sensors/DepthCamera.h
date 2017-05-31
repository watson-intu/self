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


#ifndef SELF_DEPTHCAMERA_H
#define SELF_DEPTHCAMERA_H

#include "SelfInstance.h"
#include "ISensor.h"
#include "DepthVideoData.h"

#include "utils/ThreadPool.h"
#include "utils/Time.h"

#include "SelfLib.h"

//! Base class for a 3DCamera sensor class that collects VideoData. This is not a actual implementation, see NaoCamera.
class SELF_API DepthCamera : public ISensor
{
public:
    RTTI_DECL();

    DepthCamera() : m_fFramesPerSec( 1.0f ), ISensor("DepthCamera")
    {}

    //! ISerialiazable interface
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

    //! ISensor interface
	virtual const char * GetDataType()
	{
		return "DepthVideoData";
	}
	virtual const char * GetBinaryType()
	{
		return "image/png";
	}

    virtual bool OnStart();
    virtual bool OnStop();
    virtual void OnPause();
    virtual void OnResume();

protected:
    //! Data
    float		m_fFramesPerSec;
};

#endif //SELF_DEPTHCAMERA_H
