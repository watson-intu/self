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


#ifndef SELF_PROXIMITY_EXTRACTOR_H
#define SELF_PROXIMITY_EXTRACTOR_H

#include <list>

#include "IExtractor.h"
#include "sensors/SensorManager.h"
#include "utils/Factory.h"
#include "SelfLib.h"

class SELF_API ProximityExtractor : public IExtractor
{
public:
	RTTI_DECL();

    ProximityExtractor() :
        m_MinSonarValue(0.0),
        m_MaxSonarValue(4.0),
        m_SamplesToAverage(5),
        m_CurrentAverage(0.0) 
    {}

    virtual void Deserialize(const Json::Value & json);
    virtual void Serialize(Json::Value & json);    

    //! IFeatureExtractor interface
    virtual const char * GetName() const;
    virtual bool OnStart();
    virtual bool OnStop();

private:
    //! Types
    typedef SensorManager::SensorList	SensorList;

    //! Data
    SensorList		        m_SonarSensors;
    SensorList              m_LaserSensors;
    float                   m_MinSonarValue;
    float                   m_MaxSonarValue;
    float                   m_CurrentAverage;
    int                     m_SamplesToAverage;

    //! Callback handler
    void OnSonarData(IData * data);
    void OnLaserData(IData * data);
};

#endif //SELF_PROXIMITY_EXTRACTOR_H
