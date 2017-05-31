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
#include "agent/GreeterAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"
#include "blackboard/IThing.h"
#include <string>

class TestGreeterAgent : public AgentTest
{

public:
	bool			m_bGreeterAgentTested;

	//! Construction
	TestGreeterAgent() : AgentTest("TestGreeterAgent"),
		m_bGreeterAgentTested( false )
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		GreeterAgent * pGreeterAgent = pInstance->FindAgent<GreeterAgent>();
		Test( pGreeterAgent != NULL );

		pBlackboard->SubscribeToType( "Say", DELEGATE(TestGreeterAgent, OnSay, const ThingEvent &, this) );

		Json::Value face;
		face["fullName"] = "Bob";
		face["personId"] = "5555555";

		pBlackboard->AddThing( IThing::SP( new IThing( TT_PERCEPTION, "RecognizedFace", face) ) );

		Spin( m_bGreeterAgentTested );
		Test( m_bGreeterAgentTested );

		pBlackboard->UnsubscribeFromType( "Say", this );
	}

	void OnSay( const ThingEvent & a_Event )
	{
		Say * pSay = DynamicCast<Say> ( a_Event.GetIThing().get());
		Test( pSay != NULL);

		if(pSay->GetState() == "COMPLETED")
			m_bGreeterAgentTested = true;
	}
};

TestGreeterAgent TEST_GREETER_AGENT;
