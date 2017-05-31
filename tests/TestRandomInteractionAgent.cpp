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
#include "agent/RandomInteractionAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Text.h"

class TestRandomInteractionAgent : public AgentTest
{
public:
	bool			m_bRandomInteractionAgentTested;
	float			m_fMinSpeakDelay;
	float			m_fMaxSpeakDelay;
	std::string		m_RandomInteractionKey;

	//! Construction
	TestRandomInteractionAgent() : AgentTest("TestRandomInteractionAgent"),
		m_bRandomInteractionAgentTested( false ),
		m_fMinSpeakDelay( 7.0f ),
		m_fMaxSpeakDelay( 10.0f ),
		m_RandomInteractionKey( "" )
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		RandomInteractionAgent * pRandomInteractionAgent = pInstance->FindAgent<RandomInteractionAgent>();
		Test( pRandomInteractionAgent != NULL );

		// Override the min/max delays to be occur before the spinner times out
		pRandomInteractionAgent->ResetTimeDelays( m_fMinSpeakDelay, m_fMaxSpeakDelay );

		// Get the Random Interaction phrase that will be sent to Conversation
		// to trigger a Random Interaction response
		Json::Value json;
		pRandomInteractionAgent->Serialize(json);
		Test( json.isMember("m_RandomInteractionUtterance") );
		m_RandomInteractionKey = json["m_RandomInteractionUtterance"].asString();
		Test( !m_RandomInteractionKey.empty() );

		pBlackboard->SubscribeToType( "Text", DELEGATE(TestRandomInteractionAgent, OnInteraction, const ThingEvent &, this) );
		Spin( m_bRandomInteractionAgentTested );
		Test( m_bRandomInteractionAgentTested );

		pBlackboard->UnsubscribeFromType( "Text", this );
	}

	void OnInteraction( const ThingEvent & a_Event )
	{
		Text * pText = DynamicCast<Text>( a_Event.GetIThing().get() );
		Test( pText != NULL );

		// Test that the Random Interaction trigger key was added to the Blackboard
		if ( pText->GetText() == m_RandomInteractionKey )
			m_bRandomInteractionAgentTested = true;
	}
};

TestRandomInteractionAgent TEST_RANDOM_INTERACTION_AGENT;
