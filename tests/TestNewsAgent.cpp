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
#include "agent/NewsAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"

class TestNewsAgent : public AgentTest
{
public:
	bool			m_bNewsAgentTested;

	//! Construction
	TestNewsAgent() : AgentTest("TestNewsAgent"),
		m_bNewsAgentTested(false)
	{}

	virtual void RunTest()
	{
		Log::Status("TestNewsAgent", "Starting news test");

		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test(pInstance != NULL);
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test(pBlackboard != NULL);
		NewsAgent * pNewsAgent = pInstance->FindAgent<NewsAgent>();
		Test(pNewsAgent != NULL);

		// Post display object on blackboard
		pBlackboard->SubscribeToType("NewsIntent", DELEGATE(TestNewsAgent, OnNews, const ThingEvent &, this));
		NewsIntent::SP spNewsIntent(new NewsIntent());
		spNewsIntent->SetCompany("IBM");
		pBlackboard->AddThing(spNewsIntent);

		// Check display agent reaches the completed status
		Spin(m_bNewsAgentTested, 60);
		Test(m_bNewsAgentTested);

		pBlackboard->UnsubscribeFromType("NewsIntent", this);
	}

	//Callback to see if NewsAgent reaches completed state.
	void OnNews(const ThingEvent & a_Event)
	{
		NewsIntent * pNewsIntent = DynamicCast<NewsIntent>(a_Event.GetIThing().get());
		Test(pNewsIntent != NULL);

		Log::Debug("TestNewsAgent", "this is the state: %s", pNewsIntent->GetState().c_str());
			
		if (pNewsIntent->GetState() == "COMPLETED")
			m_bNewsAgentTested = true;
	}
};

TestNewsAgent TEST_NEWS_AGENT;
