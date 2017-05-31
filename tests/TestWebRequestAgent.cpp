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
#include "agent/WebRequestAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/WebRequest.h"
#include "blackboard/Goal.h"
#include "blackboard/IThing.h"

class TestWebRequestAgent: public AgentTest
{
public:
	bool			m_bWebRequestAgentTested;

	//! Construction
	TestWebRequestAgent() : AgentTest("TestWebRequestAgent"),
		m_bWebRequestAgentTested( false )
	{}

	virtual void RunTest()
	{
		Log::Status("TestWebRequestAgent", "Starting request agent test");

		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		WebRequestAgent * pWebRequestAgent = pInstance->FindAgent<WebRequestAgent>();
		Test( pWebRequestAgent != NULL );

		// Post WebRequest object on blackboard
		pBlackboard->SubscribeToType("Goal", DELEGATE(TestWebRequestAgent, OnGoal, const ThingEvent &, this));
		std::string url = "https://www.google.com/";
		std::string type = "GET";
		std::string body = "";
		std::string params = "";
		IWebClient::Headers	 headers;
		WebRequest::SP spWebRequest( new WebRequest(url,type,body,params,headers));
		pBlackboard->AddThing( spWebRequest);

		Spin( m_bWebRequestAgentTested);
		Test( m_bWebRequestAgentTested );
		pBlackboard->UnsubscribeFromType( "Goal", this );
	}

	//check to see if an goal with request_data was added.
	void OnGoal(const ThingEvent & a_Event)
	{
		Goal * spGoal= DynamicCast<Goal>( a_Event.GetIThing().get() );
        Test( spGoal != NULL);
		Json::Value jsonValue = spGoal->GetParams().GetData();
		if (jsonValue.isMember("request_data"))
		{
				m_bWebRequestAgentTested = true;
		}
    }
};

TestWebRequestAgent TEST_WEB_REQUEST_AGENT;
