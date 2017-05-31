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


//! Define to non-zero to enable refreshing web sockets each time this self speaks
#define ENABLE_STT_REFRESH			0

#include "SpeakingAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/URL.h"
#include "skills/SkillManager.h"
#include "gestures/VolumeGesture.h"
#include "services/ILanguageTranslation.h"

REG_SERIALIZABLE(SpeakingAgent);
RTTI_IMPL(SpeakingAgent, IAgent);

SpeakingAgent::SpeakingAgent() :
	m_SpeechSkill("tts"),
	m_Language("en-US"),
	m_Gender("male"),
	m_DepressedVoice("Apology"),
	m_HappyVoice("GoodNews"),
	m_DepressedThreshold(0.33f),
	m_HappyThreshold(0.66f),
	m_EmotionalState(0.5f),
	m_bGenerateURLs(true),
	m_bExtractURLs(false),
	m_MinInterruptionSensors(1),
	m_fInterruptionSensorInterval(2.0f),
	m_BumperDebounceInterval(1.0f),
	m_bVolumeTuningFunctionality(true)
{}

void SpeakingAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
	json["m_SpeechSkill"] = m_SpeechSkill;
	json["m_Language"] = m_Language;
	json["m_Gender"] = m_Gender;
	json["m_bGenerateURLs"] = m_bGenerateURLs;
	json["m_bExtractURLs"] = m_bExtractURLs;
	json["m_MinInterruptionSensors"] = m_MinInterruptionSensors;
	json["m_fInterruptionSensorInterval"] = m_fInterruptionSensorInterval;
	json["m_BumperDebounceInterval"] = m_BumperDebounceInterval;
	json["m_bVolumeTuningFunctionality"] = m_bVolumeTuningFunctionality;
	json["m_DepressedVoice"] = m_DepressedVoice;
	json["m_HappyVoice"] = m_HappyVoice;
	json["m_DepressedThreshold"] = m_DepressedThreshold;
	json["m_HappyThreshold"] = m_HappyThreshold;

	SerializeVector("m_Interruptions", m_Interruptions, json);
	SerializeVector("m_InterruptionSensors", m_InterruptionSensors, json);
	SerializeVector("m_InterruptionResponses", m_InterruptionResponses, json);
	SerializeVector("m_VolumeTunings", m_VolumeTunings, json);
}

void SpeakingAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
	if (json.isMember("m_SpeechSkill"))
		m_SpeechSkill = json["m_SpeechSkill"].asString();
	if (json.isMember("m_Language"))
		m_Language = json["m_Language"].asString();
	if (json.isMember("m_Gender"))
		m_Gender = json["m_Gender"].asString();
	if (json.isMember("m_bGenerateURLs"))
		m_bExtractURLs = json["m_bGenerateURLs"].asBool();
	if (json.isMember("m_bExtractURLs"))
		m_bExtractURLs = json["m_bExtractURLs"].asBool();
	if (json.isMember("m_MinInterruptionSensors"))
		m_MinInterruptionSensors = json["m_MinInterruptionSensors"].asInt();
	if (json.isMember("m_fInterruptionSensorInterval"))
		m_fInterruptionSensorInterval = json["m_fInterruptionSensorInterval"].asFloat();
	if (json.isMember("m_BumperDebounceInterval"))
		m_BumperDebounceInterval = json["m_BumperDebounceInterval"].asFloat();
	if (json.isMember("m_bVolumeTuningFunctionality"))
		m_bVolumeTuningFunctionality = json["m_bVolumeTuningFunctionality"].asBool();
	if (json.isMember("m_DepressedVoice"))
		m_DepressedVoice = json["m_DepressedVoice"].asString();
	if (json.isMember("m_HappyVoice"))
		m_HappyVoice = json["m_HappyVoice"].asString();
	if (json.isMember("m_DepressedThreshold "))
		m_DepressedThreshold = json["m_DepressedThreshold"].asFloat();
	if (json.isMember("m_HappyThreshold"))
		m_HappyThreshold = json["m_HappyThreshold"].asFloat();

	DeserializeVector("m_Interruptions", json, m_Interruptions);
	DeserializeVector("m_InterruptionSensors", json, m_InterruptionSensors);
	DeserializeVector("m_InterruptionResponses", json, m_InterruptionResponses);
	DeserializeVector("m_VolumeTunings", json, m_VolumeTunings);
}

bool SpeakingAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->SubscribeToType("Say",
		DELEGATE(SpeakingAgent, OnSay, const ThingEvent &, this), TE_ADDED);
	pInstance->GetBlackBoard()->SubscribeToType("Text",
		DELEGATE(SpeakingAgent, OnText, const ThingEvent &, this), TE_ADDED);
	pInstance->GetBlackBoard()->SubscribeToType("EmotionalState",
		DELEGATE(SpeakingAgent, OnEmotionalState, const ThingEvent &, this), TE_ADDED);
	pInstance->GetBlackBoard()->SubscribeToType("TouchType",
		DELEGATE(SpeakingAgent, OnTouch, const ThingEvent &, this), TE_ADDED);

	return true;
}

bool SpeakingAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->UnsubscribeFromType("Text", this);
	pInstance->GetBlackBoard()->UnsubscribeFromType("Say", this);
	pInstance->GetBlackBoard()->UnsubscribeFromType("EmotionalState", this);
	pInstance->GetBlackBoard()->UnsubscribeFromType("TouchType", this);

	for (SensorManager::SensorList::iterator iSensor = m_TouchSensors.begin(); iSensor != m_TouchSensors.end(); ++iSensor)
		(*iSensor)->Unsubscribe(this);

	return true;
}

void SpeakingAgent::OnSay(const ThingEvent &a_ThingEvent)
{
	Say::SP spSay = DynamicCast<Say>(a_ThingEvent.GetIThing());
	if (spSay->GetText().size() > 0)
	{
		if (!m_spActive)
		{
			ExecuteSpeaking(spSay);
		}
		else
		{
			// Speaking already active, just push into the queue
			m_Speakings.push_back(spSay);
		}
	}
}

void SpeakingAgent::OnText(const ThingEvent & a_ThingEvent)
{
	// Interrupt if we have anything active
	if (m_spActive)
	{
		Text* pText = DynamicCast<Text>(a_ThingEvent.GetIThing()).get();
		if (pText != NULL)
		{
			std::string text = pText->GetText();
			for (std::vector<std::string>::const_iterator i_Interruption = m_Interruptions.begin(); i_Interruption != m_Interruptions.end(); ++i_Interruption)
			{
				if (!text.compare(*i_Interruption))
				{
					Log::Status("Conversation", "Interrupt phrase detected, aborting active speech skill.");
					SelfInstance::GetInstance()->GetSkillManager()->AbortActiveSkill(m_SpeechSkill);
					m_spActive.reset();

					if (m_InterruptionResponses.size() > 0)
					{
						SelfInstance::GetInstance()->GetBlackBoard()->AddThing(
							Say::SP(new Say(m_InterruptionResponses[rand() % m_InterruptionResponses.size()])));
					}
					break;
				}
			}
		}
	}
}

void SpeakingAgent::OnEmotionalState(const ThingEvent & a_ThingEvent)
{
	IThing::SP spThing = a_ThingEvent.GetIThing();
	if (spThing->GetDataType() == "EmotionalState")
	{
		m_EmotionalState = spThing->GetData()["m_EmotionalState"].asFloat();
	}
}

void SpeakingAgent::OnTouch(const ThingEvent & a_ThingEvent)
{
	IThing::SP spThing = a_ThingEvent.GetIThing();
	if (spThing->GetDataType() == "TouchType")
	{
		if (m_spActive && spThing->GetData()["m_TouchType"].asString() == "interrupt")
		{
			Log::Status("Conversation", "Interrupt touch detected, aborting active speech skill.");
			SelfInstance::GetInstance()->GetSkillManager()->AbortActiveSkill(m_SpeechSkill);
			m_spActive.reset();
			m_TouchSensorsEngaged.clear();
		}
		for (size_t i = 0; i < m_VolumeTunings.size(); ++i)
		{
			if (spThing->GetData()["m_TouchType"].asString() == m_VolumeTunings[i])
			{
				Log::Status("Conversation", "Volume tuning touch detected, executing skill %s", m_VolumeTunings[i].c_str());
				SelfInstance::GetInstance()->GetSkillManager()->UseSkill(m_VolumeTunings[i]);
			}
		}
	}
}

void SpeakingAgent::ExecuteSpeaking(Say::SP a_spSay)
{
	Text::SP spText = a_spSay->FindParentType<Text>(true);

	SelfInstance * pInstance = SelfInstance::GetInstance();
	Log::Status("Conversation", "Speaking Dialog: %s, TextId: %p",
		a_spSay->GetText().c_str(), spText.get());

	m_spActive = a_spSay;
	m_spActive->SetState("PROCESSING");

	std::string text(a_spSay->GetText());

	// check for language change
	if (spText && spText->GetLanguage() != m_Language)
	{
		m_Language = spText->GetLanguage();
		Log::Status("Conversation", "-------- Changing language to %s", m_Language.c_str());
	}

	// extract all tags..
	size_t nStart = text.find_first_of('[');
	while (nStart != std::string::npos)
	{
		size_t nEnd = text.find_first_of(']', nStart);
		if (nEnd == std::string::npos)
			break;

		size_t len = (nEnd - nStart) + 1;
		std::string tag = text.substr(nStart + 1, (len - 2));

		std::vector<std::string> parts;
		StringUtil::Split(tag, "=", parts);

		if (parts.size() == 2 && StringUtil::Compare(parts[0], "emote", true) == 0)
		{
			ParamsMap params;

			std::string emote = parts[1];
			size_t nParamStart = emote.find_first_of('(');
			if (nParamStart != std::string::npos)
			{
				size_t nParamEnd = emote.find_last_of(')');
				if (nParamEnd != std::string::npos)
				{
					std::vector<std::string> pairs;
					StringUtil::Split(emote.substr(nParamStart, (nParamEnd - nParamStart) + 1), "(),", pairs);
					emote.erase(nParamStart, (nParamEnd - nParamStart) + 1);

					for (size_t i = 0; i < pairs.size(); ++i)
					{
						size_t nSeperator = pairs[i].find_first_of(':');
						if (nSeperator == std::string::npos)
							continue;

						std::string key = pairs[i].substr(0, nSeperator);
						std::string value = pairs[i].substr(nSeperator + 1);
						params[key] = value;
					}
				}
				else
					Log::Error("SpeakingAgent", "Emote parameter missing ending )");
			}

			text.erase(nStart, len);
			std::string remaining = StringUtil::Trim(text.substr(nStart));
			text.erase(nStart, text.size() - nStart);
			if (!remaining.empty())
				m_Continue.push_back(Say::SP(new Say(remaining)));

			m_Emotions.push_back(Gesture::SP(new Gesture(emote, params)));

		}
		else if (parts.size() > 0 && StringUtil::Compare(parts[0], "name", true) == 0)
		{
			text.erase(nStart, len);
			text.insert(nStart, SelfInstance::GetInstance()->GetLocalConfig().m_Name);
		}
		else if (parts.size() > 0 && StringUtil::Compare(parts[0], "cache", true) == 0)
			text.erase(nStart, len);
		else
			nStart += len;

		nStart = text.find_first_of('[', nStart);
	}

	// extract URL's into URL objects
	if (m_bGenerateURLs || m_bExtractURLs)
	{
		size_t nURL = text.find("http");
		while (nURL != std::string::npos)
		{
			size_t nEnd = text.find_first_of(' ', nURL);
			if (nEnd == std::string::npos)
				nEnd = text.size() - 1;

			size_t len = (nEnd - nURL) + 1;
			std::string url = text.substr(nURL, len);
			if (m_bExtractURLs)
			{
				text.erase(nURL, len);
				nEnd = nURL;
			}

			Url::SP spURL(new Url(url));
			m_spActive->AddChild(spURL);

			Log::Status("SpeakingAgent", "Extracted URL: %s", url.c_str());
			nURL = text.find("http", nEnd);
		}
	}

	ILanguageTranslation * pTranslation = pInstance->FindService<ILanguageTranslation>();
	if (pTranslation != NULL && pTranslation->IsConfigured() && pInstance->GetBackendLanguage() != m_Language)
	{
		Log::Status("SpeakingAgent", "Translating from %s to %s: %s",
			pInstance->GetBackendLanguage().c_str(), m_Language.c_str(), text.c_str());
		pTranslation->Translation(pInstance->GetBackendLanguage(), m_Language,
			text, DELEGATE(SpeakingAgent, OnTranslate, Translations *, this));
	}
	else
	{
		text = StringUtil::Trim(text);
		if (text.size() > 0)
		{
			if (pInstance->GetTopics()->GetSubscriberCount("conversation") > 0)
				pInstance->GetTopics()->Publish("conversation", text);

			ParamsMap params;
			params["text"] = text;
			if (m_EmotionalState < m_DepressedThreshold)
				params["emotion"] = m_DepressedVoice;
			else if (m_EmotionalState > m_HappyThreshold)
				params["emotion"] = m_HappyVoice;
			else
				params["emotion"] = "";

			params["language"] = m_Language;
			params["gender"] = m_Gender;

			pInstance->GetSkillManager()->UseSkill(m_SpeechSkill, params,
				DELEGATE(SpeakingAgent, OnSkillState, SkillInstance *, this), m_spActive);
#if ENABLE_STT_REFRESH
			if (m_pSTT != NULL)
				m_pSTT->RefreshConnections();
#endif
		}
		else
			OnSpeakingDone();
	}
}

void SpeakingAgent::OnTranslate(Translations * a_Callback)
{
	std::string text;
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (a_Callback == NULL)
	{
		Log::Error("SpeakingAgent", "Translation Failed, Text: %s, Language: %s",
			m_spActive->GetText().c_str(), m_Language.c_str());
		text = m_spActive->GetText();
	}
	else
		text = a_Callback->m_Translations[0].m_Translation;

	text = StringUtil::Trim(text);
	if (text.size() > 0)
	{
		if (pInstance->GetTopics()->GetSubscriberCount("conversation") > 0)
			pInstance->GetTopics()->Publish("conversation", text);

		ParamsMap params;
		params["text"] = text;
		params["language"] = m_Language;
		params["gender"] = m_Gender;

		pInstance->GetSkillManager()->UseSkill(m_SpeechSkill, params,
			DELEGATE(SpeakingAgent, OnSkillState, SkillInstance *, this), m_spActive);
#if ENABLE_STT_REFRESH
		if (m_pSTT != NULL)
			m_pSTT->RefreshConnections();
#endif
	}
	else
		OnSpeakingDone();
}

void SpeakingAgent::OnSkillState(SkillInstance * a_pInstance)
{
	SkillInstance::UseSkillState state = a_pInstance->GetState();
	const std::string& stateName = a_pInstance->StateToString(state);

	// Update the state
	m_spActive->SetState(stateName);

	switch (state)
	{
		// the phrase has been either completed or aborted - continue to next phrase if available
	case SkillInstance::US_COMPLETED:
	case SkillInstance::US_ABORTED:
		OnSpeakingDone();
		break;

		// Something went wrong with the skill - report the failure
	case SkillInstance::US_FAILED:
	case SkillInstance::US_UNAVAILABLE:
		m_Speakings.clear();
		m_spActive.reset();
		break;

		// the skill is current is working, let him continue
	case SkillInstance::US_FINDING:
	case SkillInstance::US_EXECUTING:
		// No op. We logged that already, there is nothing else to do
		break;

	case SkillInstance::US_INVALID:
		m_spActive.reset();
		break;
	}
}

void SpeakingAgent::OnSpeakingDone()
{
	while (m_Emotions.begin() != m_Emotions.end())
	{
		Gesture::SP spEmotion = m_Emotions.front();
		m_Emotions.pop_front();

		m_spActive->AddChild(spEmotion);
	}

	while (m_Continue.begin() != m_Continue.end())
	{
		Say::SP spSay = m_Continue.front();
		m_Continue.pop_front();

		m_spActive->AddChild(spSay);
	}

	if (m_Speakings.begin() != m_Speakings.end())
	{
		Say::SP spSay = m_Speakings.front();
		m_Speakings.pop_front();

		ExecuteSpeaking(spSay);
	}
	else
		m_spActive.reset();
}
