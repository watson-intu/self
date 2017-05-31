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
#include "agent/SpeakingAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"

class TestSpeakingAgent : public AgentTest
{
public:
	bool			m_bSpeakingAgentTested;

	//! Construction
	TestSpeakingAgent() : AgentTest("TestSpeakingAgent"),
		m_bSpeakingAgentTested( false )
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		SpeakingAgent * pSpeakingAgent = pInstance->FindAgent<SpeakingAgent>();
		Test( pSpeakingAgent != NULL );

		pBlackboard->SubscribeToType( "Say", DELEGATE(TestSpeakingAgent, OnSay, const ThingEvent &, this) );
		Say::SP spSay( new Say("Hello World") );
		pBlackboard->AddThing( spSay );		
		Spin( m_bSpeakingAgentTested );
		Test( m_bSpeakingAgentTested );

		pBlackboard->UnsubscribeFromType( "Say", this );
	}

	void OnSay( const ThingEvent & a_Event )
	{
		Say * pSay = DynamicCast<Say>( a_Event.GetIThing().get() );
		Test( pSay != NULL );

		if ( pSay->GetState() == "COMPLETED" )
			m_bSpeakingAgentTested = true;
	}
};

TestSpeakingAgent TEST_SPEAKING_AGENT;
