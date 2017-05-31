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


#include "tinyxml/tinyxml.h"

#include "QuestionAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Health.h"
#include "blackboard/QuestionIntent.h"
#include "blackboard/HangOnIntent.h"
#include "blackboard/Say.h"
#include "blackboard/Status.h"
#include "blackboard/Goal.h"
#include "blackboard/Text.h"
#include "utils/TimerPool.h"
#include "utils/StringHash.h"
#include "utils/Path.h"
#include "utils/JsonHelpers.h"
#include "tinyxml/tinyxml.h"
#include "SelfInstance.h"

#pragma warning(disable:4996)

REG_SERIALIZABLE(QuestionAgent);
RTTI_IMPL(QuestionAgent, IAgent);
RTTI_IMPL(IQuestionAnswerProxy, ISerializable);

QuestionAgent::QuestionAgent() :
	m_MinDialogConfidence(0.0),
	m_MinAnswerConfidence(0.0),
	m_UseDialogConfidence(0.5),
	m_nQuestionLimit(3)
{}

void QuestionAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);

	json["m_MinDialogConfidence"] = m_MinDialogConfidence;
	json["m_MinAnswerConfidence"] = m_MinAnswerConfidence;
	json["m_UseDialogConfidence"] = m_UseDialogConfidence;
	json["m_nQuestionLimit"] = m_nQuestionLimit;

	SerializeVector("m_QuestionAnswerProxies", m_QuestionAnswerProxies, json);
	SerializeVector("m_PipelineDownResponses", m_PipelineDownResponses, json);
}

void QuestionAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);

	if (json.isMember("m_MinDialogConfidence"))
		m_UseDialogConfidence = json["m_MinDialogConfidence"].asDouble();
	if (json.isMember("m_MinAnswerConfidence"))
		m_UseDialogConfidence = json["m_MinAnswerConfidence"].asDouble();
	if (json.isMember("m_UseDialogConfidence"))
		m_UseDialogConfidence = json["m_UseDialogConfidence"].asDouble();
	if (json.isMember("m_nQuestionLimit"))
		m_nQuestionLimit = json["m_nQuestionLimit"].asInt();

	DeserializeVector("m_QuestionAnswerProxies", json, m_QuestionAnswerProxies);
	DeserializeVector("m_PipelineDownResponses", json, m_PipelineDownResponses);

	if (m_PipelineDownResponses.size() == 0)
		m_PipelineDownResponses.push_back("I'm sorry, but the %s pipeline seems to be down.");
}

bool QuestionAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->SubscribeToType("QuestionIntent",
		DELEGATE(QuestionAgent, OnQuestionIntent, const ThingEvent &, this), TE_ADDED);

	for (size_t i = 0; i < m_QuestionAnswerProxies.size(); ++i)
		m_QuestionAnswerProxies[i]->Start();

	return true;
}

bool QuestionAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->UnsubscribeFromType("QuestionIntent", this);

	for (size_t i = 0; i < m_QuestionAnswerProxies.size(); ++i)
		m_QuestionAnswerProxies[i]->Stop();

	return true;
}

void QuestionAgent::OnQuestionIntent(const ThingEvent & a_ThingEvent)
{
	QuestionIntent::SP spQuestion = DynamicCast<QuestionIntent>(a_ThingEvent.GetIThing());

	if (spQuestion)
	{
		if (!spQuestion->GetGoalParams().isNull())
		{
			Goal::SP spGoal(new Goal());
			spGoal->SetName("Question");
			spGoal->SetParams(spQuestion->GetGoalParams());
			spQuestion->AddChild(spGoal);
		}
		else if (m_ActiveQuestions.size() < (size_t)m_nQuestionLimit)
		{
			// process the question, this object handles the various callbacks..
			new ProcessQuestion(this, spQuestion);
		}
		else
		{
			Log::Status("QuestionAgent", "Already answering maximum number of questions %d/%d",
				m_ActiveQuestions.size(), m_nQuestionLimit);
		}
	}
}

//-----------------------------------------------------
// ProcessQuestion

QuestionAgent::ProcessQuestion::ProcessQuestion(QuestionAgent * a_pAgent, QuestionIntent::SP a_spQuestion) :
	m_pAgent(a_pAgent), m_spQuestion(a_spQuestion), m_PendingRequests(0), m_CompletedRequests(0), m_bGoalCreated(false)
{
	m_pAgent->m_ActiveQuestions.push_back(this);
	m_spQuestion->SetState("PROCESSING");

	m_PendingRequests += 1;
	for (size_t i = 0; i < m_pAgent->m_QuestionAnswerProxies.size(); ++i)
	{
		m_PendingRequests += 1;
		m_pAgent->m_QuestionAnswerProxies[i]->AskQuestion(m_spQuestion,
			DELEGATE(ProcessQuestion, OnAnswer, const Json::Value &, this));
	}
	m_CompletedRequests += 1;

	if (m_CompletedRequests == m_PendingRequests)
		OnFinalResult();
}

QuestionAgent::ProcessQuestion::~ProcessQuestion()
{
	m_pAgent->m_ActiveQuestions.remove(this);
}

void QuestionAgent::ProcessQuestion::OnAnswer(const Json::Value & json)
{
	m_CompletedRequests += 1;
	double confidence = 0.0;

	if ((!json.isNull()) && json.isMember("response") && json.isMember("confidence"))
	{
		Json::Value answer = json;

		// Convert any XML data in the response into JSON 
		Json::Value & responses = answer["response"];
		for (size_t i = 0; i < responses.size(); ++i)
		{
			if (!responses[i].isString())
				continue;
			const std::string & response = responses[i].asString();

			TiXmlDocument xml;
			xml.Parse(response.c_str());

			if (!xml.Error())
			{
				Json::Value json;
				JsonHelpers::MakeJSON(xml.FirstChildElement(), json);
				//Log::Debug( "QuestionAgent", "Json: %s", json.toStyledString().c_str() );
				responses[i] = json;
			}
		}

		Log::Debug("Conversation", "DialogResponse: %s", answer.toStyledString().c_str());
		m_Responses.push_back(answer);

		// if priorityQA and confidence is high enough, use the dialog immediately without waiting for all responses
		if (answer.isMember("hasPriority") && answer["hasPriority"].asBool())
			confidence = answer["confidence"].asDouble();
	}
	else
	{
		Log::Error("QuestionAgent", "NULL or improperly formatted JSON returned");
	}

	if (confidence >= m_pAgent->m_UseDialogConfidence || m_CompletedRequests == m_PendingRequests)
		OnFinalResult();
}

void QuestionAgent::ProcessQuestion::OnFinalResult()
{
	if (!m_bGoalCreated && !m_spQuestion->IsLocalDialog())
	{
		m_bGoalCreated = true;

		// Iterate through all responses here....
		double bestConfidence = 0.0;
		int bestIndex = -1;
		for (size_t i = 0; i < m_Responses.size(); i++)
		{
			double tempConfidence = m_Responses[i]["confidence"].asDouble();
			if (tempConfidence > bestConfidence)
			{
				bestConfidence = tempConfidence;
				bestIndex = i;
			}
		}
		if (bestConfidence > m_pAgent->m_MinAnswerConfidence)
		{
			Json::Value answer = m_Responses[bestIndex];
			m_Responses.clear();

			Goal::SP spGoal(new Goal());
			spGoal->SetName("Question");

			ParamsMap & params = spGoal->GetParams();
			params["intent"] = "question";
			params["question"] = m_spQuestion->ToJson();
			params["answer"] = answer;

			m_spQuestion->AddChild(spGoal);
		}
		else
		{
			// failed to find anything with a high enough confidence.
			// Just make a new goal and be done with it..
			Goal::SP spGoal(new Goal());
			spGoal->SetName("Question");

			ParamsMap & params = spGoal->GetParams();
			params["intent"] = "question";
			params["question"] = m_spQuestion->ToJson();
			if (bestIndex >= 0)
				params["missedAnswer"] = m_Responses[bestIndex];
			//params["missedDialog"] = m_DialogResponse;

			m_spQuestion->AddChild(spGoal);
		}

		m_spQuestion->SetState("COMPLETED");
	}

	if (m_CompletedRequests == m_PendingRequests)
		delete this;
}

