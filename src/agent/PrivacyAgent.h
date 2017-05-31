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


#ifndef SELF_PRIVACYAGENT_H
#define SELF_PRIVACYAGENT_H

#include "IAgent.h"
#include "sensors/SensorManager.h"
#include "SelfLib.h"

class SELF_API PrivacyAgent : public IAgent
{
public:
    RTTI_DECL();

    PrivacyAgent() : m_LogSpeech( false ), m_LogCamera( false ), m_EnableDynamicOptOut( false ), m_bStoreAudio( false),
                     m_MinimumAge( 25 ), m_AgeTimeout(30.0f), m_CameraOffIntent( "camera_off" ), m_CameraOnIntent( "camera_on" )
    {}

    //! ISerializable interface
    void Serialize( Json::Value & json );
    void Deserialize( const Json::Value & json );

    //! IAgent interface
    virtual bool OnStart();
    virtual bool OnStop();

private:

    //! Types
    typedef SensorManager::SensorList	SensorList;

    //! Data
    bool		m_LogSpeech;
    bool		m_LogCamera;
    bool		m_EnableDynamicOptOut;
    bool		m_bStoreAudio;
    int			m_MinimumAge;
    float		m_AgeTimeout;
	std::string	m_CameraOffIntent;
	std::string m_CameraOnIntent;
	SensorList	m_ActiveCameras;

    TimerPool::ITimer::SP
            m_spAgeTimeout;

    void    OnPerson(const ThingEvent & a_ThingEvent);
	void	OnProxyIntent(const ThingEvent & a_ThingEvent);
	void	SetCameraActivity(bool isActive);
    void    OnAgeTimeout();
    void    AddPrivacyThing(const ThingEvent & a_ThingEvent);
	void	OnAddVideoSensor(ISensor * a_pSensor);
	void	OnRemoveVideoSensor(ISensor * a_pSensor);
};

#endif //SELF_PRIVACYAGENT_H
