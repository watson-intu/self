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
#include "agent/ThinkingAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"
#include "blackboard/IIntent.h"

class TestIntent: public IIntent
{
public:
	//! Construction
	TestIntent() : IIntent()
	{}
	//IIntent interface
	virtual void Create(const Json::Value & a_Intent, const Json::Value & a_Parse)
	{}
};

class TestThinkingAgent: public AgentTest
{
public:
	bool			m_bThinkingAgentTested;

	//! Construction
	TestThinkingAgent() : AgentTest("TestThinkingAgent"),
		m_bThinkingAgentTested( false )
	{}

	virtual void RunTest()
	{
		Log::Status("TestThinkingAgent", "Starting ThinkingAgent test");

		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		ThinkingAgent * pThinkingAgent = pInstance->FindAgent<ThinkingAgent>();
		Test( pThinkingAgent != NULL );

		// Post TestIntent object on blackboard and subscribe to Say
		pBlackboard->SubscribeToType( "Say", DELEGATE(TestThinkingAgent, OnSayUmm, const ThingEvent &, this) );
		TestIntent::SP spThinking( new TestIntent() );
		pBlackboard->AddThing( spThinking );
		spThinking->SetState("PROCESSING");

		Spin( m_bThinkingAgentTested);
		Test( m_bThinkingAgentTested );

		pBlackboard->UnsubscribeFromType( "Say", this );
	}

	void OnSayUmm( const ThingEvent & a_Event )
	{
		Log::Status("TestThinkingAgent", "Entering onUmmm");
		Say * pSay = DynamicCast<Say>( a_Event.GetIThing().get() );
		Test( pSay != NULL );

		if ( pSay->GetText() == "ummmm...." )
			m_bThinkingAgentTested = true;
	}

};

TestThinkingAgent TEST_THINKING_AGENT;
