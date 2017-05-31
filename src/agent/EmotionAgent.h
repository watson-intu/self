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


#ifndef SELF_EMOTION_AGENT_H
#define SELF_EMOTION_AGENT_H

#include "IAgent.h"
#include "blackboard/Text.h"
#include "blackboard/LearningIntent.h"
#include "services/IToneAnalyzer.h"
#include "sensors/SensorManager.h"
#include "sensors/MoodData.h"
#include "utils/IConditional.h"
#include "SelfLib.h"

class SkillInstance;

//! This agent handles emotions
class SELF_API EmotionAgent : public IAgent
{
public:
	RTTI_DECL();

	//! Types
	typedef std::vector<IConditional::SP>		Conditions;

	struct TouchResponse : public ISerializable
	{
		RTTI_DECL();

		//! ISerializable interface
		virtual void Serialize(Json::Value & json);
		virtual void Deserialize(const Json::Value & json);

		Conditions				m_Conditions;			// conditions to fire this response
		std::string				m_Emotion;				// emotional response to this touch
	};

	//! Constructions
	EmotionAgent() : 
		m_SaySomething(false), 
		m_WaitTime( 30.0f ), 
		m_EmotionalState( 0.5f ), 
		m_WeatherWaitTime( 3600.0f ), 
		m_EmotionTime( 30.0f ), 
		m_LastEmotionalState( 0.0f )
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

	const float GetEmotionalState() const
	{
		return m_EmotionalState;
	}

private:

	//! Types
	typedef SensorManager::SensorList   SensorList;
	typedef std::vector<TouchResponse>	TouchResponses;

	//! Data
	SensorList					m_MoodSensors;

	bool 						m_SaySomething;
	float 						m_WaitTime;
	float						m_WeatherWaitTime;
	float						m_EmotionalState;
	float						m_LastEmotionalState;
	float						m_EmotionTime;
	TimerPool::ITimer::SP		m_spWaitTimer;
	TimerPool::ITimer::SP		m_spWeatherTimer;
	TimerPool::ITimer::SP		m_spEmotionTimer;
	std::vector<std::string>	m_NegativeTones;
	std::vector<std::string>	m_PositiveTones;
	std::vector<std::string>	m_PositiveWeather;
	std::vector<std::string>	m_NegativeWeather;
	TouchResponses				m_TouchResponses;

	//! Event Handlers
	void		OnText(const ThingEvent & a_ThingEvent);
	void		OnLearningIntent(const ThingEvent & a_ThingEvent);
	void		OnWeatherData(const Json::Value & json);
	void		OnTone(DocumentTones * a_Callback);
	void		OnTouchData(const ThingEvent & a_ThingEvent);

	void		OnAddMoodSensor( ISensor * a_pSensor );
	void		OnRemoveMoodSensor( ISensor * a_pSensor );
	void        OnMoodData(IData * a_pData);
	

	void		OnEnableEmotion();
	void		OnEnableWeather();
	void		OnEmotionCheck();
	void		PublishEmotionalState();
};

#endif // EMOTION Agent

