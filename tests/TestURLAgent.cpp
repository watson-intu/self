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
#include "agent/URLAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/URL.h"


class TestURLAgent: public AgentTest
{
public:
	bool			m_bURLAgentTested;

	//! Construction
	TestURLAgent() : AgentTest("TestURLAgent"),
		m_bURLAgentTested( false )
	{}

	virtual void RunTest()
	{
		Log::Status("TestURLAgent", "Starting URL test");

		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		URLAgent * pURLAgent = pInstance->FindAgent<URLAgent>();
		Test( pURLAgent != NULL );

		// Post URL object on blackboard
		pBlackboard->SubscribeToType( "Url", DELEGATE(TestURLAgent, OnURL, const ThingEvent &, this) );
		Url::SP spURL (new Url() );
		spURL->SetURL("www.google.com");
		pBlackboard->AddThing( spURL );

		// Check URL agent reaches the completed status
		Spin( m_bURLAgentTested, 60.0f );
		Test( m_bURLAgentTested);

		pBlackboard->UnsubscribeFromType( "Url", this );
	}

	void OnURL( const ThingEvent & a_Event )
	{
		Url * pURL = DynamicCast<Url>( a_Event.GetIThing().get() );
		Test( pURL != NULL );
		// TODO: Since not all platforms do not have a display that is accepting a URL this test would fail on most devices and fail an automated build. The allowing "FAILED" below should be removed and a future multi-host solution should be added.
		if ( pURL->GetState() == "COMPLETED" || pURL->GetState() == "FAILED")
		{
			m_bURLAgentTested = true;
		}
	}
};

TestURLAgent TEST_URL_AGENT;
