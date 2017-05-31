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


#include "SkillTeachingAgent.h"
#include "SelfInstance.h"
#include "classifiers/TextClassifier.h"
#include "skills/SkillManager.h"
#include "skills/LinearSkill.h"
#include "blackboard/LearningIntent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Goal.h"
#include "blackboard/Say.h"
#include "blackboard/Text.h"

REG_SERIALIZABLE(SkillTeachingAgent);
RTTI_IMPL(SkillTeachingAgent, IAgent);

const double FIND_TEXT_TIME = 60.0f;

SkillTeachingAgent::SkillTeachingAgent() : m_bEnableSkillTraining(true), m_bEnableNLCTraining(false)
{}

void SkillTeachingAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);

	SerializeVector("m_LearningResponses", m_LearningResponses, json);
	SerializeVector("m_ForgetResponses", m_ForgetResponses, json);
	SerializeVector("m_ConfirmRetrain", m_ConfirmRetrain, json);
	SerializeVector("m_RetrainResponses", m_RetrainResponses, json);

	json["m_bEnableSkillTraining"] = m_bEnableSkillTraining;
	json["m_bEnableNLCTraining"] = m_bEnableNLCTraining;
}

void SkillTeachingAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);

	DeserializeVector("m_LearningResponses", json, m_LearningResponses);
	DeserializeVector("m_ForgetResponses", json, m_ForgetResponses);
	DeserializeVector("m_ConfirmRetrain", json, m_ConfirmRetrain);
	DeserializeVector("m_RetrainResponses", json, m_RetrainResponses);
	if (json.isMember("m_bEnableSkillTraining"))
		m_bEnableSkillTraining = json["m_bEnableSkillTraining"].asBool();
	if (json.isMember("m_bEnableNLCTraining"))
		m_bEnableNLCTraining = json["m_bEnableNLCTraining"].asBool();

	if (m_LearningResponses.size() == 0)
		m_LearningResponses.push_back("Now I know how to %s.");
	if (m_ForgetResponses.size() == 0)
		m_ForgetResponses.push_back("I have forgotten how to %s.");
	if (m_ConfirmRetrain.size() == 0)
		m_ConfirmRetrain.push_back("Please confirm that you want to retrain the phrase, %s, as a %s?");
	if (m_RetrainResponses.size() == 0)
		m_RetrainResponses.push_back("I will now understand the phrase, %s, as a %s.");
}

bool SkillTeachingAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	pBlackboard->SubscribeToType("Goal",
		DELEGATE(SkillTeachingAgent, OnGoal, const ThingEvent &, this));
	pBlackboard->SubscribeToType("UsedSkill",
		DELEGATE(SkillTeachingAgent, OnSkillUsed, const ThingEvent &, this), TE_ADDED);
	pBlackboard->SubscribeToType("LearningIntent",
		DELEGATE(SkillTeachingAgent, OnLearningIntent, const ThingEvent &, this), TE_ADDED);
	pBlackboard->SubscribeToType("Text",
		DELEGATE(SkillTeachingAgent, OnText, const ThingEvent &, this), TE_ADDED);
	pBlackboard->SubscribeToType("Confirm",
		DELEGATE(SkillTeachingAgent, OnConfirmRetrain, const ThingEvent &, this), TE_STATE);

	return true;
}

bool SkillTeachingAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	pBlackboard->UnsubscribeFromType("Goal", this);
	pBlackboard->UnsubscribeFromType("UsedSkill", this);
	pBlackboard->UnsubscribeFromType("LearningIntent", this);
	pBlackboard->UnsubscribeFromType("Text", this);
	pBlackboard->UnsubscribeFromType("Confirm", this);

	return true;
}

void SkillTeachingAgent::OnGoal(const ThingEvent & a_ThingEvent)
{
	Goal::SP spGoal = DynamicCast<Goal>(a_ThingEvent.GetIThing());
	if (spGoal)
	{
		if (spGoal->GetState() == "FAILED")
		{
			Log::Debug("SkillTeachingAgent", "Tracking last failed goal %s", spGoal->GetName().c_str());
			m_spLastFailedGoal = spGoal;
			m_UsedSkills.clear();
		}
	}
}

void SkillTeachingAgent::OnSkillUsed(const ThingEvent & a_ThingEvent)
{
	if (m_spLastFailedGoal)
	{
		UsedSkill::SP spUsedSkill = DynamicCast<UsedSkill>(a_ThingEvent.GetIThing());
		if (spUsedSkill && !spUsedSkill->IsParent(m_spLastFailedGoal))
		{
			Log::Debug("SkillTeachingAgent", "Used skill: %s", spUsedSkill->GetSkill()->GetSkillName().c_str());
			m_UsedSkills.push_back(spUsedSkill);
		}
	}
}

void SkillTeachingAgent::OnText(const ThingEvent & a_ThingEvent)
{
	Text::SP spText = DynamicCast<Text>(a_ThingEvent.GetIThing());
	if (spText)
	{
		// keep a very short memory of text transcripts, we only need to know the previous
		// one from the current one.
		m_TextHistory.push_back(spText);
		while (m_TextHistory.size() > 2)
			m_TextHistory.pop_front();
	}
}

void SkillTeachingAgent::OnLearningIntent(const ThingEvent & a_ThingEvent)
{
	LearningIntent::SP spIntent = DynamicCast<LearningIntent>(a_ThingEvent.GetIThing());

	if (spIntent)
	{
		Log::Status("SkillTeachingAgent", "LearningIntent received, status = %s", spIntent->GetVerb().c_str());
		if (m_bEnableSkillTraining)
		{
			if (spIntent->GetVerb() == "learn_skill" || spIntent->GetVerb() == "completed_learning")
				OnLearnSkill(spIntent);
			else if (spIntent->GetVerb() == "forget_skill")
				OnForgetSkill(spIntent);
		}
		if (m_bEnableNLCTraining)
		{
			if (spIntent->GetVerb() == "learn_command")
				OnRetrainNLC(spIntent, "command");
			else if (spIntent->GetVerb() == "learn_question")
				OnRetrainNLC(spIntent, "question");
			else if (spIntent->GetVerb() == "learn_statement")
				OnRetrainNLC(spIntent, "statement");
		}
	}

}

void SkillTeachingAgent::OnLearnSkill(LearningIntent::SP a_spIntent)
{
	IThing::ThingList failedGoals;

	SkillManager * pSkillManager = SelfInstance::GetInstance()->GetSkillManager();
	if (m_spLastFailedGoal && m_UsedSkills.begin() != m_UsedSkills.end())
	{
		LinearSkill::SP spLearnedSkill(new LinearSkill());
		spLearnedSkill->SetSkillName(m_spLastFailedGoal->GetName());

		for (std::list< UsedSkill::SP >::iterator iUsed = m_UsedSkills.begin(); iUsed != m_UsedSkills.end(); ++iUsed)
			spLearnedSkill->AddSkill((*iUsed)->GetSkill());

		pSkillManager->AddSkill(spLearnedSkill);

		if (m_LearningResponses.size() > 0)
		{
			std::string response(StringUtil::Format(m_LearningResponses[rand() % m_LearningResponses.size()].c_str(),
				spLearnedSkill->GetSkillName().c_str()));
			a_spIntent->AddChild(Say::SP(new Say(response)));
			a_spIntent->SetState("COMPLETED");
		}

		m_spLastFailedGoal.reset();
		m_UsedSkills.clear();
	}
	else
	{
		Log::Status("SkillTeachingAgent", "No failed goals found or skills used, verb: %s, target: %s.",
			a_spIntent->GetVerb().c_str(), a_spIntent->GetTarget().c_str());
	}
}

void SkillTeachingAgent::OnForgetSkill(LearningIntent::SP a_spIntent)
{
	SkillManager * pSkillManager = SelfInstance::GetInstance()->GetSkillManager();
	if (pSkillManager->DeleteSkill(a_spIntent->GetTarget()))
	{
		Log::Debug("LearningGoal", "Successfully forgot how to: %s", a_spIntent->GetTarget().c_str());
		if (m_LearningResponses.size() > 0)
		{
			std::string response(StringUtil::Format(m_ForgetResponses[rand() % m_ForgetResponses.size()].c_str(), a_spIntent->GetTarget().c_str()));
			a_spIntent->AddChild(Say::SP(new Say(response)));
			a_spIntent->SetState("COMPLETED");
		}
	}
	else
	{
		Log::Warning("LearningGoal", "Could not delete skill: %s", a_spIntent->GetTarget().c_str());
	}
}

void SkillTeachingAgent::OnRetrainNLC(LearningIntent::SP a_spIntent, const std::string & a_Class)
{
	if (m_TextHistory.size() == 2)
	{
		Text::SP spPreviousText = m_TextHistory.front();
		Log::Debug("SkillTeachingAgent", "Retraining NLC, Text: %s, Class: %s",
			spPreviousText->GetText().c_str(), a_Class.c_str());

		std::string response(StringUtil::Format(m_ConfirmRetrain[rand() % m_ConfirmRetrain.size()].c_str(),
			spPreviousText->GetText().c_str(), a_Class.c_str()));
		a_spIntent->AddChild(Say::SP(new Say(response)));

		Json::Value info;
		info["text"] = spPreviousText->GetText();
		info["class"] = a_Class;

		Confirm::SP spConfirm(new Confirm("SkillTeaching", info));
		a_spIntent->AddChild(spConfirm);
	}
}

void SkillTeachingAgent::OnConfirmRetrain(const ThingEvent & a_ThingEvent)
{
	Confirm::SP spConfirm = DynamicCast<Confirm>(a_ThingEvent.GetIThing());
	if (spConfirm && spConfirm->GetConfirmType() == "SkillTeaching")
	{
		// TODO
	}
}