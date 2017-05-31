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


#ifndef SELF_SPEAKING_AGENT_H
#define SELF_SPEAKING_AGENT_H

#include "IAgent.h"
#include "blackboard/Say.h"
#include "blackboard/Text.h"
#include "blackboard/Gesture.h"
#include "sensors/TouchData.h"
#include "sensors/SensorManager.h"
#include "sensors/ISensor.h"
#include "utils/Time.h"
#include "SelfLib.h"

class SkillInstance;
struct Translations;

//! This agent handles speaking to the users
class SELF_API SpeakingAgent : public IAgent
{
public:
	RTTI_DECL();

	SpeakingAgent();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Types
	typedef std::list<Say::SP>		SpeakingList;
	typedef std::list<Gesture::SP>	EmotionList;

	//! Data
	std::string                 m_SpeechSkill;
	std::string					m_Language;
	std::string					m_Gender;
	bool						m_bGenerateURLs;
	bool						m_bExtractURLs;

	std::vector<std::string>    m_Interruptions;
	std::vector<std::string>    m_InterruptionSensors;
	std::vector<std::string>    m_InterruptionResponses;
	std::vector<std::string>	m_VolumeTunings;

	SensorManager::SensorList   m_TouchSensors;
	std::vector<std::string>    m_TouchSensorsEngaged;
	Time                        m_LastSensorEngageTime;
	int                         m_MinInterruptionSensors;
	float                       m_fInterruptionSensorInterval;

	Time 						m_BumperLastTime;
	float 						m_BumperDebounceInterval;
	float						m_EmotionalState;
	float						m_DepressedThreshold;
	float						m_HappyThreshold;
	std::string					m_DepressedVoice;
	std::string					m_HappyVoice;
	bool                        m_bVolumeTuningFunctionality;

	Say::SP	                    m_spActive;
	SpeakingList                m_Speakings;
	EmotionList                 m_Emotions;
	SpeakingList				m_Continue;

	//! Event Handlers
	void		OnSay( const ThingEvent &a_ThingEvent );
	void 		OnText( const ThingEvent & a_ThingEvent );
	void		OnEmotionalState( const ThingEvent & a_ThingEvent );
	void		OnTouch( const ThingEvent & a_ThingEvent );
	void		ExecuteSpeaking( Say::SP a_spSay );
	void		OnSkillState( SkillInstance * a_pSkill );
	void 		OnTranslate(Translations * a_pTranslations);
	void		OnSpeakingDone();
};

#endif // SELF_SPEAKING_AGENT_H