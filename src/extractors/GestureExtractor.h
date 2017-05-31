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

#ifndef SELF_GESTUREEXTRACTOR_H
#define SELF_GESTUREEXTRACTOR_H

#include <list>

#include "IExtractor.h"
#include "sensors/SensorManager.h"
#include "utils/Factory.h"
#include "SelfLib.h"


class SELF_API GestureExtractor : public IExtractor
{
public:
RTTI_DECL();

    GestureExtractor() {}


    //! IFeatureExtractor interface
    virtual const char * GetName() const;
    virtual bool OnStart();
    virtual bool OnStop();

    private:
    //! Types
    typedef SensorManager::SensorList	SensorList;

    //! Data
    SensorList		        m_GazeSensors;
    SensorList              m_GestureSensors;

    //! Callback handler
    void OnGazeData(IData * data);
    void OnGestureData(IData * data);

};

#endif //SELF_GESTUREEXTRACTOR_H
