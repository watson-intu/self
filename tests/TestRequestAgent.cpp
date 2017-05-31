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
#include "agent/RequestAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/RequestIntent.h"
#include "blackboard/Goal.h"
#include "blackboard/IThing.h"

class TestRequestAgent: public AgentTest
{
public:
	bool			m_bRequestAgentTested;

	//! Construction
	TestRequestAgent() : AgentTest("TestRequestAgent"),
		m_bRequestAgentTested( false )
	{}

	virtual void RunTest()
	{
		Log::Status("TestRequestAgent", "Starting request agent test");

		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		RequestAgent * pRequestAgent = pInstance->FindAgent<RequestAgent>();
		Test( pRequestAgent != NULL );

		// Post RequestIntent object on blackboard
		pBlackboard->SubscribeToType("Goal", DELEGATE(TestRequestAgent, OnGoal, const ThingEvent &, this));

		RequestIntent::SP spRequestIntent( new RequestIntent());
		std::string verb = "wave";
		std::string target = "crowd";
		Log::Status("TestRequestAgent", "Adding RequestIntent with verb: %s and target: %s", verb.c_str(), target.c_str());
		spRequestIntent->AddRequest(verb, target);
		pBlackboard->AddThing( spRequestIntent );

		// Check if an goal with "Time" is on the blackboard
		Spin( m_bRequestAgentTested);
		Test( m_bRequestAgentTested );

		pBlackboard->UnsubscribeFromType( "Goal", this );
	}

	void OnGoal(const ThingEvent & a_Event) 
	{
		Goal * spGoal= DynamicCast<Goal>( a_Event.GetIThing().get() );
        Test( spGoal != NULL);
		Json::Value jsonValue = spGoal->GetParams().GetData();
		if (jsonValue.isMember("intent") && jsonValue.isMember("verb") && jsonValue.isMember("target") )
		{
			if(jsonValue["intent"] == "request")
			{
				m_bRequestAgentTested = true;
			}
		}
    }
};

TestRequestAgent TEST_REQUEST_AGENT;
