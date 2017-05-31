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


#include "AgentTest.h"
#include "SelfInstance.h"
#include "agent/FeedbackAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Text.h"
#include "blackboard/LearningIntent.h"

class TestFeedbackAgent : public AgentTest
{
public:
	bool			m_bFeedbackAgentTested;
	std::string		m_InputText;
	std::string		m_TargetSentiment;

	//! Construction
	TestFeedbackAgent() : AgentTest("TestFeedbackAgent"),
		m_InputText("You suck"),
		m_TargetSentiment("negative_feedback"),
		m_bFeedbackAgentTested( false )
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		FeedbackAgent * pFeedbackAgent = pInstance->FindAgent<FeedbackAgent>();
		Test( pFeedbackAgent != NULL );

		pBlackboard->SubscribeToType( "LearningIntent", DELEGATE(TestFeedbackAgent, OnLearningIntent, const ThingEvent &, this) );
		Text::SP spText( new Text(m_InputText, .99, false, true) );
		pBlackboard->AddThing( spText );
		Spin( m_bFeedbackAgentTested );
		Test( m_bFeedbackAgentTested );

		pBlackboard->UnsubscribeFromType( "LearningIntent", this );
	}

	void OnLearningIntent( const ThingEvent & a_Event )
	{
		LearningIntent * pLearningIntent = DynamicCast<LearningIntent>( a_Event.GetIThing().get() );
		Test( pLearningIntent != NULL );

		if( pLearningIntent->GetText() == m_InputText )
		{
			if( pLearningIntent->GetTarget() == m_TargetSentiment && pLearningIntent->GetVerb() == "feedback" )
				m_bFeedbackAgentTested = true;
		}
	}
};

TestFeedbackAgent TEST_FEEDBACK_AGENT;
