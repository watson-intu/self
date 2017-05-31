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
#include "agent/TimeAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"
#include "blackboard/Goal.h"

class TestTimeAgent: public AgentTest
{
public:
	bool			m_bTimeAgentTested;

	//! Construction
	TestTimeAgent() : AgentTest("TestTimeAgent"),
		m_bTimeAgentTested( false )
	{}

	virtual void RunTest()
	{
		Log::Status("TestTimeAgent", "Starting display test");

		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		TimeAgent * pTimeAgent = pInstance->FindAgent<TimeAgent>();
		Test( pTimeAgent != NULL );

		// Post TimeIntent object on blackboard
		pBlackboard->SubscribeToType( "Goal", DELEGATE(TestTimeAgent, OnGoal, const ThingEvent &, this) );
		TimeIntent::SP spTimeIntent( new TimeIntent() );
		pBlackboard->AddThing( spTimeIntent );

		// Check if an goal with "Time" is on the blackboard
		Spin( m_bTimeAgentTested);
		Test( m_bTimeAgentTested );

		pBlackboard->UnsubscribeFromType( "Goal", this );
	}

	//Callback to see if TimeAgent added goal to blackboard.
	void OnGoal( const ThingEvent & a_Event )
	{
		Goal * pGoal = DynamicCast<Goal>( a_Event.GetIThing().get() );
		Test( pGoal != NULL );

		if ( pGoal->GetName() == "Time" )
			m_bTimeAgentTested = true;
	}
};

TestTimeAgent TEST_TIME_AGENT;
