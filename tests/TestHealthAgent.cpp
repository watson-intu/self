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
#include "agent/HealthAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Health.h"
#include "blackboard/Goal.h"

class TestHealthAgent : public AgentTest
{
public:
	bool			m_bHealthAgentTested;
	bool			m_bHealthObjectCreated;
	std::string		m_HealthName;

	//! Construction
	TestHealthAgent() : AgentTest("TestHealthAgent"),
		m_bHealthAgentTested( false ),
		m_bHealthObjectCreated( false ),
		m_HealthName("NetworkFailure")
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		HealthAgent * pHealthAgent = pInstance->FindAgent<HealthAgent>();
		Test( pHealthAgent != NULL );

		pBlackboard->SubscribeToType( "Health", DELEGATE(TestHealthAgent, OnHealth, const ThingEvent &, this) );
		pBlackboard->SubscribeToType( "Goal", DELEGATE(TestHealthAgent, OnGoal, const ThingEvent &, this) );

		Health::SP spHealth( new Health( "Joints", m_HealthName, "CRITICAL", 0.5, false, true ) );
		pBlackboard->AddThing( spHealth );

		Spin( m_bHealthAgentTested );
		Test( m_bHealthObjectCreated );
		Test( m_bHealthAgentTested );

		pBlackboard->UnsubscribeFromType( "Health", this );
		pBlackboard->UnsubscribeFromType( "Goal", this );
	}

	void OnHealth( const ThingEvent & a_Event )
	{
		Health * pHealth = DynamicCast<Health>( a_Event.GetIThing().get() );
		Test( pHealth != NULL );

		// Test that the Health object was created
		if( pHealth->GetHealthName() == m_HealthName )
			m_bHealthObjectCreated = true;
	}

	void OnGoal( const ThingEvent & a_Event )
	{
		Goal * pGoal = DynamicCast<Goal>( a_Event.GetIThing().get() );

		// Test that a goal was created for the Health object and is completed
		if( pGoal->GetName() == m_HealthName && pGoal->GetState() == "COMPLETED" )
			m_bHealthAgentTested = true;
	}
};

TestHealthAgent TEST_HEALTH_AGENT;
