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


#include "FeedbackAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/LearningIntent.h"
#include "blackboard/QuestionIntent.h"
#include "blackboard/RequestIntent.h"
#include "blackboard/Say.h"

REG_SERIALIZABLE(FeedbackAgent);
RTTI_IMPL( FeedbackAgent, IAgent);

void FeedbackAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);

	SerializeVector( "m_PositiveResponses", m_PositiveResponses, json );
	SerializeVector( "m_NegativeResponses", m_NegativeResponses, json );
}

void FeedbackAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);

	DeserializeVector( "m_PositiveResponses", json, m_PositiveResponses);
	if (m_PositiveResponses.size() == 0 )
		m_PositiveResponses.push_back( "Glad to hear that!" );
	DeserializeVector( "m_NegativeResponses", json, m_NegativeResponses);
	if (m_NegativeResponses.size() == 0 )
		m_NegativeResponses.push_back( "Sorry to hear that!" );
}

bool FeedbackAgent::OnStart()
{
	SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType(LearningIntent::GetStaticRTTI(),
		DELEGATE(FeedbackAgent, OnLearningIntent, const ThingEvent &, this), TE_ADDED);
	SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType(RequestIntent::GetStaticRTTI(),
		DELEGATE(FeedbackAgent, OnIntents, const ThingEvent &, this), TE_ADDED);
	SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType(QuestionIntent::GetStaticRTTI(),
		DELEGATE(FeedbackAgent, OnIntents, const ThingEvent &, this), TE_ADDED);
	SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType(Confirm::GetStaticRTTI(),
		DELEGATE(FeedbackAgent, OnConfirm, const ThingEvent &, this), TE_ADDED);

	return true;
}

bool FeedbackAgent::OnStop()
{
	BlackBoard * pBlackboard = SelfInstance::GetInstance()->GetBlackBoard();
	pBlackboard->UnsubscribeFromType(LearningIntent::GetStaticRTTI(), this);
	pBlackboard->UnsubscribeFromType(RequestIntent::GetStaticRTTI(), this);
	pBlackboard->UnsubscribeFromType(QuestionIntent::GetStaticRTTI(), this);
	pBlackboard->UnsubscribeFromType(Confirm::GetStaticRTTI(), this);

	return true;
}

void FeedbackAgent::OnLearningIntent(const ThingEvent & a_ThingEvent)
{
	LearningIntent::SP spIntent = DynamicCast<LearningIntent>(a_ThingEvent.GetIThing());
	if (spIntent && spIntent->GetVerb().compare("feedback") == 0 )
	{
		Log::Debug("FeedbackAgent", "FeedbackAgent Intent: %s", a_ThingEvent.GetIThing()->GetRTTI().GetName().c_str());

		if (m_Confirmations.size() > 0)
		{
			Confirm::SP spConfirm = m_Confirmations.front();
			m_Confirmations.pop_front();

			if (spIntent->GetTarget().compare("positive_feedback") == 0)
			{
				spConfirm->OnConfirmed();
			}
			else
			{
				spConfirm->OnCancelled();
			}
		}
		else if (m_spLastIntent)
		{
			// TODO: send positive/negative feedback somewhere to actually do something
			m_spLastIntent.reset();
		}
		spIntent->AddChild(Say::SP(new Say(spIntent->GetAnswer())));
	}
}

void FeedbackAgent::OnIntents(const ThingEvent & a_ThingEvent)
{
	IIntent::SP spIntent = DynamicCast<IIntent>(a_ThingEvent.GetIThing());
	if (spIntent)
		m_spLastIntent = spIntent;
}

void FeedbackAgent::OnConfirm(const ThingEvent & a_ThingEvent)
{
	Confirm::SP spConfirm = DynamicCast<Confirm>(a_ThingEvent.GetIThing());
	if (spConfirm)
		m_Confirmations.push_back(spConfirm);
}
