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
#include "agent/DisplayAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"

class TestDisplayAgent: public AgentTest
{
public:
	bool			m_bDisplayAgentTested;

	//! Construction
	TestDisplayAgent() : AgentTest("TestDisplayAgent"),
		m_bDisplayAgentTested( false )
	{}

	virtual void RunTest()
	{
		Log::Status("TestDisplayAgent", "Starting display test");

		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		DisplayAgent * pDisplayAgent = pInstance->FindAgent<DisplayAgent>();
		Test( pDisplayAgent != NULL );

		// Post display object on blackboard
		pBlackboard->SubscribeToType( "Display", DELEGATE(TestDisplayAgent, OnDisplay, const ThingEvent &, this) );
		Display::SP spDisplay( new Display() );
		spDisplay->SetDisplay("First part of display object added to the blackboard");
		spDisplay->SetData("Second part of display object added to the blackboard");
		pBlackboard->AddThing( spDisplay );

		// Check display agent reaches the completed status
		Spin( m_bDisplayAgentTested);
		Test( m_bDisplayAgentTested );

		pBlackboard->UnsubscribeFromType( "Display", this );
	}

	//Callback to see if DisplayAgent reaches completed state.
	void OnDisplay( const ThingEvent & a_Event )
	{
		Display * pDisplay = DynamicCast<Display>( a_Event.GetIThing().get() );
		Test( pDisplay != NULL );

		if ( pDisplay->GetState() == "COMPLETED" )
			m_bDisplayAgentTested = true;
	}
};

TestDisplayAgent TEST_DISPLAY_AGENT;
