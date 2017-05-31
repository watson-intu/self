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
#include "agent/PrivacyAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Person.h"
#include "blackboard/IThing.h"

class TestPrivacyAgent: public AgentTest
{
public:
	bool			m_bRequestAgentTested;

	//! Construction
	TestPrivacyAgent() : AgentTest("TestPrivacyAgent"),
		m_bRequestAgentTested( false )
	{}

	virtual void RunTest()
	{
		// This test will only pass if the below two variables are set to true
		// "m_EnableDynamicOptOut" : true
		// "m_bStoreAudio" : true
		// In the file /etc/tests/profile/body.json

		Log::Status("TestPrivacyAgent", "Starting privacy agent test");

		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		PrivacyAgent * pPrivacyAgent= pInstance->FindAgent<PrivacyAgent>();
		Test( pPrivacyAgent!= NULL );

		pBlackboard->SubscribeToType("Privacy", DELEGATE(TestPrivacyAgent, OnPrivacyAdded, const ThingEvent &, this));
		Person::SP spPerson(new Person());
		spPerson->SetGender("male");
		spPerson->SetAgeRange("01-02");
		pBlackboard->AddThing( spPerson );

		Spin( m_bRequestAgentTested);
		Test( m_bRequestAgentTested );

		pBlackboard->UnsubscribeFromType( "Privacy", this );
	}

	void OnPrivacyAdded(const ThingEvent & a_Event)
	{
		IThing * spIThing= DynamicCast<IThing>( a_Event.GetIThing().get() );
        Test( spIThing!= NULL);
		if (spIThing->GetDataType() == "Privacy")
		{
			m_bRequestAgentTested = true;
		}
    }
};

TestPrivacyAgent TEST_PRIVACY_AGENT;
