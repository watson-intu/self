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
#include "agent/QuestionAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Text.h"
#include "blackboard/Goal.h"
#include "blackboard/QuestionIntent.h"

class TestQuestionAgent : public AgentTest
{
public:
	bool			m_bQuestionAgentTested;
	bool 			m_bQuestionIntentCreated;
	std::string		m_QuestionText;

	//! Construction
	TestQuestionAgent() : AgentTest("TestQuestionAgent"),
		m_bQuestionAgentTested( false ),
		m_bQuestionIntentCreated( false ),
		m_QuestionText("how are you")
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		QuestionAgent * pQuestionAgent = pInstance->FindAgent<QuestionAgent>();
		Test( pQuestionAgent != NULL );

		pBlackboard->SubscribeToType( "QuestionIntent", DELEGATE(TestQuestionAgent, OnQuestion, const ThingEvent &, this) );
		pBlackboard->SubscribeToType( "Goal", DELEGATE(TestQuestionAgent, OnGoal, const ThingEvent &, this) );
		Text::SP spText( new Text(m_QuestionText, .99, false, true) );
		pBlackboard->AddThing( spText );

		Spin( m_bQuestionAgentTested );
		Test( m_bQuestionIntentCreated );
		Test( m_bQuestionAgentTested );

		pBlackboard->UnsubscribeFromType( "QuestionIntent", this );
		pBlackboard->UnsubscribeFromType( "Goal", this );
	}

	void OnQuestion( const ThingEvent & a_Event )
	{
		QuestionIntent * pQuestion = DynamicCast<QuestionIntent>( a_Event.GetIThing().get() );
		Test( pQuestion != NULL );

		Test( !pQuestion->GetText().empty() );
		if( pQuestion->GetText() == m_QuestionText )
			m_bQuestionIntentCreated = true;
	}

	void OnGoal( const ThingEvent & a_Event )
	{
		Goal * pGoal = DynamicCast<Goal>( a_Event.GetIThing().get() );
		if( pGoal->GetName() == "Question" )
		{
			Json::Value data = pGoal->GetParams().GetData();
			if( data.isMember("question") && data["question"].isMember("m_Text") )
			{
				std::string question = data["question"]["m_Text"].asString();

				if (question == m_QuestionText && pGoal->GetState() == "COMPLETED")
					m_bQuestionAgentTested = true;
			}
		}
	}
};

TestQuestionAgent TEST_QUESTION_AGENT;
