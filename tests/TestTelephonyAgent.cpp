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
#include "agent/TelephonyAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Text.h"
#include "blackboard/Goal.h"
#include "blackboard/TelephonyIntent.h"

class TestTelephonyAgent : public AgentTest
{
public:
	bool			m_bTelephonyAgentTested;
	bool 			m_bTelephonyGoalTested;

	//! Construction
	TestTelephonyAgent() : AgentTest("TestTelephonyAgent"),
		m_bTelephonyAgentTested( false ),
		m_bTelephonyGoalTested( false )
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		TelephonyAgent * pTelephonyAgent = pInstance->FindAgent<TelephonyAgent>();
		Test( pTelephonyAgent != NULL );

		pBlackboard->SubscribeToType( "TelephonyIntent", DELEGATE(TestTelephonyAgent, OnTelephony, const ThingEvent &, this) );
		pBlackboard->SubscribeToType( "Goal", DELEGATE(TestTelephonyAgent, OnGoal, const ThingEvent &, this) );

		Text::SP spText( new Text("Call Micah", .99, false, true) );
		pBlackboard->AddThing( spText );
		Spin( m_bTelephonyAgentTested );

		Test( m_bTelephonyAgentTested );
		Test( m_bTelephonyGoalTested );

		pBlackboard->UnsubscribeFromType( "Telephony", this );
		pBlackboard->UnsubscribeFromType( "Goal", this );
	}

	void OnTelephony( const ThingEvent & a_Event )
	{
		TelephonyIntent * pTelephony = DynamicCast<TelephonyIntent>( a_Event.GetIThing().get() );
		Test( pTelephony != NULL );

		if ( pTelephony->GetState() == "PROCESSING" )
			m_bTelephonyAgentTested = true;
	}

	void OnGoal( const ThingEvent & a_Event )
	{
		Goal * pGoal = DynamicCast<Goal>( a_Event.GetIThing().get() );
		Test( pGoal != NULL );

		if ( pGoal->GetName() == "OutgoingCall" )
			m_bTelephonyGoalTested = true;
	}
};

TestTelephonyAgent TEST_TELEPHONY_AGENT;
