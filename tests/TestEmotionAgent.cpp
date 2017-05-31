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
#include "blackboard/BlackBoard.h"
#include "agent/EmotionAgent.h"
#include "blackboard/Gesture.h"
#include "blackboard/Text.h"

class TestEmotionAgent : public AgentTest
{
public:
	bool			m_bEmotionAgentTested;
	float 			m_fEmotionalStateStart;
	float 			m_fEmotionalStateEnd;

	//! Construction
	TestEmotionAgent() : AgentTest("TestEmotionAgent"),
		m_bEmotionAgentTested( false ),
		m_fEmotionalStateStart( 0.0 ),
		m_fEmotionalStateEnd( 0.0 )
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		EmotionAgent * pEmotionAgent = pInstance->FindAgent<EmotionAgent>();
		Test( pEmotionAgent != NULL );

		m_fEmotionalStateStart = pEmotionAgent->GetEmotionalState();
		pBlackboard->SubscribeToType( "LearningIntent", DELEGATE(TestEmotionAgent, OnLearningIntent, const ThingEvent &, this) );

		Text::SP spText( new Text("good job", .99, false, true) );
		pBlackboard->AddThing( spText );

		Spin( m_bEmotionAgentTested );
		Test( m_bEmotionAgentTested );

		//m_fEmotionalStateEnd = pEmotionAgent->GetEmotionalState();
		//Test( m_fEmotionalStateEnd > m_fEmotionalStateStart );

		pBlackboard->UnsubscribeFromType( "LearningIntent", this );
	}

	void OnLearningIntent( const ThingEvent & a_Event )
	{
		m_bEmotionAgentTested = true;
	}
};

TestEmotionAgent TEST_EMOTION_AGENT;
