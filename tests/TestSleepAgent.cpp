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
#include "agent/SleepAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Health.h"

class TestSleepAgent : public AgentTest
{
public:
	bool			m_bSleepAgentTested;

	//! Construction
	TestSleepAgent() : AgentTest("TestSleepAgent"),
	                  m_bSleepAgentTested( false )
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		SleepAgent * pSleepAgent = pInstance->FindAgent<SleepAgent>();
		Test( pSleepAgent != NULL );

		pBlackboard->SubscribeToType( "Health", DELEGATE(TestSleepAgent, OnHealth, const ThingEvent &, this) );

		Health::SP spHealth( new Health( "Joints", "Diagnosis/Temperature/Test", "CRITICAL", 0.0f, true, false ) );
		pBlackboard->AddThing( spHealth );
		Spin( m_bSleepAgentTested );
		Test( m_bSleepAgentTested );

		pBlackboard->UnsubscribeFromType( "Health", this );
	}

	void OnHealth( const ThingEvent & a_Event )
	{
		Health * pHealth = DynamicCast<Health>( a_Event.GetIThing().get() );
		Test( pHealth != NULL );

		if ( pHealth->GetHealthName() == "Rest" )
			m_bSleepAgentTested = true;
	}
};

TestSleepAgent TEST_SLEEP_AGENT;
