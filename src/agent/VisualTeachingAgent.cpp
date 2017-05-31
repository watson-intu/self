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


#include "VisualTeachingAgent.h"
#include "SelfInstance.h"
#include "classifiers/ImageClassifier.h"
#include "skills/SkillManager.h"
#include "blackboard/LearningIntent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"
#include "blackboard/Image.h"

REG_SERIALIZABLE(VisualTeachingAgent);
RTTI_IMPL(VisualTeachingAgent, IAgent);

const unsigned int MIN_TRAINING_IMAGES = 10;

VisualTeachingAgent::VisualTeachingAgent() :
	m_bEnableObjectTraining(true),
	m_TrainingImageCount(10)
{}

void VisualTeachingAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);

	SerializeVector("m_LearnObjectResponses", m_LearnObjectResponses, json);
	SerializeVector("m_ForgetObjectResponses", m_ForgetObjectResponses, json);
	SerializeVector("m_FailedImageTraining", m_FailedImageTraining, json);
	json["m_bEnableObjectTraining"] = m_bEnableObjectTraining;
	json["m_TrainingImageCount"] = m_TrainingImageCount;
}

void VisualTeachingAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);

	if (json.isMember("m_bEnableObjectTraining"))
		m_bEnableObjectTraining = json["m_bEnableObjectTraining"].asBool();
	if (json.isMember("m_TrainingImageCount"))
		m_TrainingImageCount = json["m_TrainingImageCount"].asUInt();
	if (json.isMember("m_LearnObjectResponses"))
		DeserializeVector("m_LearnObjectResponses", json, m_LearnObjectResponses);
	if (json.isMember("m_ForgetObjectResponses"))
		DeserializeVector("m_ForgetObjectResponses", json, m_ForgetObjectResponses);
	if (json.isMember("m_FailedImageTraining"))
		DeserializeVector("m_FailedImageTraining", json, m_FailedImageTraining);

	if (m_LearnObjectResponses.size() == 0)
		m_LearnObjectResponses.push_back("This object is new to me, I will try to remember it as a %s.");
	if (m_ForgetObjectResponses.size() == 0)
		m_ForgetObjectResponses.push_back("I will remember that this is a negative example of a %s.");
	if (m_FailedImageTraining.size() == 0)
		m_FailedImageTraining.push_back("I cannot currently train on images due to an error");
}

bool VisualTeachingAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	pBlackboard->SubscribeToType("LearningIntent",
		DELEGATE(VisualTeachingAgent, OnLearningIntent, const ThingEvent &, this), TE_ADDED);
	pBlackboard->SubscribeToType("Image",
		DELEGATE(VisualTeachingAgent, OnImage, const ThingEvent &, this), TE_ADDED);

	return true;
}

bool VisualTeachingAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	pBlackboard->UnsubscribeFromType("LearningIntent", this);
	pBlackboard->UnsubscribeFromType("Image", this);

	return true;
}

void VisualTeachingAgent::OnImage(const ThingEvent & a_ThingEvent)
{
	Image::SP spImage = DynamicCast<Image>(a_ThingEvent.GetIThing());
	if (spImage)
	{
		m_ImageHistory.push_back(spImage);

		// free any images beyond what we need for training..
		while (m_ImageHistory.size() > m_TrainingImageCount)
			m_ImageHistory.pop_front();
	}
}

void VisualTeachingAgent::OnLearningIntent(const ThingEvent & a_ThingEvent)
{
	LearningIntent::SP spIntent = DynamicCast<LearningIntent>(a_ThingEvent.GetIThing());
	if (spIntent)
	{
		m_bEnableObjectTraining = true; // This is only temporary
		if (m_bEnableObjectTraining)
		{
			if (spIntent->GetVerb() == "learn_object")
				OnLearnObject(spIntent);
			else if (spIntent->GetVerb() == "forget_object")
				OnForgetObject(spIntent);
		}
	}
}

void VisualTeachingAgent::OnLearnObject(LearningIntent::SP a_spIntent)
{
	ImageClassifier * pClassifier = SelfInstance::GetInstance()->FindClassifier<ImageClassifier>();
	if (pClassifier != NULL)
	{
		if (m_ImageHistory.size() >= m_TrainingImageCount && m_ImageHistory.size() >= MIN_TRAINING_IMAGES)
		{
			std::string target = a_spIntent->GetTarget();
			if (target.size() > 0)
			{
				StringUtil::Replace(target, "a ", "");
				StringUtil::Replace(target, " ", "_");
				StringUtil::ToLower(target);

				pClassifier->SubmitPositiveImages(m_ImageHistory, target);

				std::string response(StringUtil::Format(m_LearnObjectResponses[rand() % m_LearnObjectResponses.size()].c_str(), target.c_str()));
				a_spIntent->AddChild(Say::SP(new Say(response)));
			}
			else
				Log::Error("VisualTechingAgent", "Target is empty for OnLearnObject.");
		}
	}
}

void VisualTeachingAgent::OnForgetObject(LearningIntent::SP a_spIntent)
{
	ImageClassifier * pClassifier = SelfInstance::GetInstance()->FindClassifier<ImageClassifier>();
	if (pClassifier != NULL)
	{
		if (m_ImageHistory.size() >= m_TrainingImageCount && m_ImageHistory.size() >= MIN_TRAINING_IMAGES)
		{
			std::string target = a_spIntent->GetTarget();
			if (target.size() > 0)
			{
				StringUtil::Replace(target, "a ", "");
				StringUtil::Replace(target, " ", "_");
				StringUtil::ToLower(target);

				pClassifier->SubmitNegativeImages(m_ImageHistory, target);

				std::string response(StringUtil::Format(m_ForgetObjectResponses[rand() % m_ForgetObjectResponses.size()].c_str(), target.c_str()));
				a_spIntent->AddChild(Say::SP(new Say(response)));
			}
			else
				Log::Error("VisualTechingAgent", "Target is empty for OnForgetObject.");
		}
	}
}