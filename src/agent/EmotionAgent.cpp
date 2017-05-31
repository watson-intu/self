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


#include "EmotionAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Goal.h"
#include "skills/SkillManager.h"
#include "sensors/TouchData.h"
#include "services/IWeather.h"

RTTI_IMPL_EMBEDDED(EmotionAgent, TouchResponse, ISerializable);

void EmotionAgent::TouchResponse::Serialize(Json::Value & json)
{
	SerializeVector("m_Conditions", m_Conditions, json);
	json["m_Emotion"] = m_Emotion;
}

void EmotionAgent::TouchResponse::Deserialize(const Json::Value & json)
{
	DeserializeVector("m_Conditions", json, m_Conditions);
	m_Emotion = json["m_Emotion"].asString();
}

REG_SERIALIZABLE(EmotionAgent);
RTTI_IMPL(EmotionAgent, IAgent);

void EmotionAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);

	SerializeVector("m_NegativeTones", m_NegativeTones, json);
	SerializeVector("m_PositiveTones", m_PositiveTones, json);
	SerializeVector("m_NegativeWeather", m_NegativeWeather, json);
	SerializeVector("m_PositiveWeather", m_PositiveWeather, json);
	SerializeVector("m_TouchResponses", m_TouchResponses, json);

	json["m_WaitTime"] = m_WaitTime;
	json["m_WeatherWaitTime"] = m_WeatherWaitTime;
	json["m_EmotionalState"] = m_EmotionalState;
	json["m_EmotionTime"] = m_EmotionTime;
}

void EmotionAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);

	DeserializeVector("m_NegativeTones", json, m_NegativeTones);
	DeserializeVector("m_PositiveTones", json, m_PositiveTones);
	DeserializeVector("m_NegativeWeather", json, m_NegativeWeather);
	DeserializeVector("m_PositiveWeather", json, m_PositiveWeather);
	DeserializeVector("m_TouchResponses", json, m_TouchResponses);

	if (json.isMember("m_WaitTime"))
		m_WaitTime = json["m_WaitTime"].asFloat();
	if (json.isMember("m_WeatherWaitTime"))
		m_WeatherWaitTime = json["m_WeatherWaitTime"].asFloat();
	if (json.isMember("m_EmotionalState"))
		m_EmotionalState = json["m_EmotionalState"].asFloat();
	if (json.isMember("m_EmotionTime"))
		m_EmotionTime = json["m_EmotionTime"].asFloat();

	if (m_NegativeTones.size() == 0)
		m_NegativeTones.push_back("extraversion");
	if (m_PositiveTones.size() == 0)
		m_PositiveTones.push_back("joy");
	if (m_NegativeWeather.size() == 0)
		m_NegativeWeather.push_back("Cloudy");
	if (m_PositiveWeather.size() == 0)
		m_PositiveWeather.push_back("Clear");
}

bool EmotionAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert(pInstance != NULL);
	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert(pBlackboard != NULL);

	pBlackboard->SubscribeToType("Text",
		DELEGATE(EmotionAgent, OnText, const ThingEvent &, this), TE_ADDED);
	pBlackboard->SubscribeToType("LearningIntent",
		DELEGATE(EmotionAgent, OnLearningIntent, const ThingEvent &, this), TE_ADDED);
	pBlackboard->SubscribeToType("TouchType",
		DELEGATE(EmotionAgent, OnTouchData, const ThingEvent &, this), TE_ADDED);

	pInstance->GetSensorManager()->RegisterForSensor("MoodData",
		DELEGATE(EmotionAgent, OnAddMoodSensor, ISensor *, this),
		DELEGATE(EmotionAgent, OnRemoveMoodSensor, ISensor *, this));

	TimerPool * pTimerPool = TimerPool::Instance();
	if (pTimerPool != NULL)
	{
		m_spWaitTimer = pTimerPool->StartTimer(VOID_DELEGATE(EmotionAgent, OnEnableEmotion, this), m_WaitTime, true, true);
		m_spWeatherTimer = pTimerPool->StartTimer(VOID_DELEGATE(EmotionAgent, OnEnableWeather, this), m_WeatherWaitTime, true, true);
		m_spEmotionTimer = pTimerPool->StartTimer(VOID_DELEGATE(EmotionAgent, OnEmotionCheck, this), m_EmotionTime, true, true);
	}
	return true;
}

bool EmotionAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert(pInstance != NULL);
	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert(pBlackboard != NULL);

	pBlackboard->UnsubscribeFromType("Text", this);
	pBlackboard->UnsubscribeFromType("LearningIntent", this);
	pBlackboard->UnsubscribeFromType("TouchType", this);


	for (size_t i = 0; i < m_MoodSensors.size(); ++i)
		m_MoodSensors[i]->Unsubscribe(this);
	m_MoodSensors.clear();

	pInstance->GetSensorManager()->UnregisterForSensor("MoodData", this);

	m_spWaitTimer.reset();
	m_spWeatherTimer.reset();
	m_spEmotionTimer.reset();
	return true;
}

void EmotionAgent::OnText(const ThingEvent & a_ThingEvent)
{
	Text::SP spText = DynamicCast<Text>(a_ThingEvent.GetIThing());
	if (spText)
	{
		IToneAnalyzer * pToneAnalyzer = SelfInstance::GetInstance()->FindService<IToneAnalyzer>();
		if (pToneAnalyzer != NULL && pToneAnalyzer->IsConfigured())
		{
			boost::shared_ptr<EmotionAgent> spThis(boost::static_pointer_cast<EmotionAgent>(shared_from_this()));
			pToneAnalyzer->GetTone(spText->GetText(), DELEGATE(EmotionAgent, OnTone, DocumentTones *, spThis));
		}
	}
}

void EmotionAgent::OnLearningIntent(const ThingEvent & a_ThingEvent)
{
	LearningIntent::SP spLearningIntent = DynamicCast<LearningIntent>(a_ThingEvent.GetIThing());
	if (spLearningIntent && spLearningIntent->GetVerb().compare("feedback") == 0)
	{
		if (spLearningIntent->GetTarget().compare("positive_feedback") == 0)
		{
			if (m_EmotionalState < 1.0)
				m_EmotionalState += 0.1f;
		}
		else
		{
			if (m_EmotionalState > 0.0)
				m_EmotionalState -= 0.1f;
		}

		PublishEmotionalState();
	}
}

void EmotionAgent::OnEnableWeather()
{
	IWeather * pWeather = SelfInstance::GetInstance()->FindService<IWeather>();
	if (pWeather != NULL && pWeather->IsConfigured())
	{
		boost::shared_ptr<EmotionAgent> spThis(boost::static_pointer_cast<EmotionAgent>(shared_from_this()));
		pWeather->GetCurrentConditions(NULL, DELEGATE(EmotionAgent, OnWeatherData, const Json::Value &, spThis));
	}
	else
		m_spWeatherTimer.reset();
}

void EmotionAgent::OnWeatherData(const Json::Value & json)
{
	if (json["forecasts"].isArray() && json["forecasts"].size() > 0)
	{
		Json::Value forecasts = json["forecasts"][0];

		std::string skyCover;
		if (forecasts.isMember("phrase_32char"))
			skyCover = forecasts["phrase_32char"].asString();

		bool weatherMatch = false;
		for (size_t i = 0; i < m_PositiveWeather.size(); ++i)
		{
			if (skyCover == m_PositiveWeather[i])
			{
				if (m_EmotionalState < 1.0f)
					m_EmotionalState += 0.1f;
				weatherMatch = true;
				break;
			}
		}

		if (!weatherMatch)
		{
			for (size_t i = 0; i < m_NegativeWeather.size(); ++i)
			{
				if (skyCover == m_NegativeWeather[i])
				{
					if (m_EmotionalState > 0.0f)
						m_EmotionalState -= 0.1f;
				}
			}
		}

		PublishEmotionalState();
	}
}

void EmotionAgent::OnTone(DocumentTones * a_Callback)
{
	if (a_Callback != NULL)
	{
		double topScore = 0.0;
		Tone tone;
		for (size_t i = 0; i < a_Callback->m_ToneCategories.size(); ++i)
		{
			for (size_t j = 0; j < a_Callback->m_ToneCategories[i].m_Tones.size(); ++j)
			{
				Tone someTone = a_Callback->m_ToneCategories[i].m_Tones[j];
				if (someTone.m_Score > topScore)
				{
					topScore = someTone.m_Score;
					tone = someTone;
				}
			}
		}
		Log::Debug("EmotionAgent", "Found top tone as: %s", tone.m_ToneName.c_str());
		bool toneFound = false;
		for (size_t i = 0; i < m_PositiveTones.size(); ++i)
		{
			if (tone.m_ToneId == m_PositiveTones[i])
			{
				toneFound = true;
				if (m_EmotionalState < 1.0f)
					m_EmotionalState += 0.1f;
			}
		}

		if (!toneFound)
		{
			for (size_t i = 0; i < m_NegativeTones.size(); ++i)
			{
				if (tone.m_ToneId == m_NegativeTones[i])
				{
					if (m_EmotionalState > 0.0f)
						m_EmotionalState -= 0.1f;
				}
			}
		}

		PublishEmotionalState();
	}

	delete a_Callback;
}

void EmotionAgent::OnAddMoodSensor(ISensor * a_pSensor)
{
	m_MoodSensors.push_back(a_pSensor->shared_from_this());
	a_pSensor->Subscribe(DELEGATE(EmotionAgent, OnMoodData, IData *, this));
}

void EmotionAgent::OnRemoveMoodSensor(ISensor * a_pSensor)
{
	for (size_t i = 0; i < m_MoodSensors.size(); ++i)
		if (m_MoodSensors[i].get() == a_pSensor)
		{
			a_pSensor->Unsubscribe(this);
			m_MoodSensors.erase(m_MoodSensors.begin() + i);
			break;
		}
}

void EmotionAgent::OnMoodData(IData * data)
{
	MoodData * pMood = DynamicCast<MoodData>(data);
	if (pMood != NULL && m_SaySomething)
	{
		// TODO: Our emotion should be based on the MoodData coming into this agent
		Goal::SP spGoal(new Goal("Emotion"));
		spGoal->GetParams()["emotion"] = "show_laugh";
		SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spGoal);

		m_SaySomething = false;
	}
}

void EmotionAgent::OnTouchData(const ThingEvent & a_ThingEvent)
{
	IThing::SP spThing = a_ThingEvent.GetIThing();
	if (spThing)
	{
		Json::Value touch;
		touch["m_EmotionalState"] = m_EmotionalState;
		touch["m_TouchType"] = spThing->GetData()["m_TouchType"].asString();

		Log::Debug("EmotionAgent", "Touch Detected: %s", touch.toStyledString().c_str());

		for (size_t i = 0; i < m_TouchResponses.size(); ++i)
		{
			TouchResponse & response = m_TouchResponses[i];
			Log::Debug("EmotionAgent", "Testing response %u.", i);

			bool bEmote = true;
			for (size_t k = 0; k < response.m_Conditions.size() && bEmote; ++k)
			{
				if (!response.m_Conditions[k]->Test(touch))
				{
					Log::Debug("EmotionAgent", "Condition %u failed.", k);
					bEmote = false;
				}
			}

			if (bEmote)
			{
				Log::Debug("EmotionAgent", "Firing touch response %s", response.m_Emotion.c_str());

				Goal::SP spGoal(new Goal("Emotion"));
				spGoal->GetParams()["emotion"] = response.m_Emotion;

				SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spGoal);
			}
		}
	}
}

void EmotionAgent::OnEnableEmotion()
{
	m_SaySomething = true;
}

void EmotionAgent::OnEmotionCheck()
{
	if (m_EmotionalState > 0.5)
		m_EmotionalState -= 0.1f;
	else
		m_EmotionalState += 0.1f;

	PublishEmotionalState();
}

void EmotionAgent::PublishEmotionalState()
{
	if (m_EmotionalState != m_LastEmotionalState)
	{
		SelfInstance::GetInstance()->GetBlackBoard()->AddThing(IThing::SP(
			new IThing(TT_PERCEPTION, "EmotionalState", IThing::JsonObject("m_EmotionalState", m_EmotionalState), 3600.0f)));
		m_LastEmotionalState = m_EmotionalState;
	}
}
