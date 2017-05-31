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
#include "agent/VisualTeachingAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/LearningIntent.h"
#include "blackboard/Goal.h"
#include "blackboard/IThing.h"
#include "blackboard/Say.h"
#include "blackboard/Image.h"

class TestVisualTeachingAgent: public AgentTest {
public:
	int m_VisualTeachingAgentTestedCount;

	//! Construction
	TestVisualTeachingAgent() : AgentTest("TestVisualTeachingAgent"),
								m_VisualTeachingAgentTestedCount(0) { }

	virtual void RunTest()
	{
		Log::Status("TestVisualTeachingAgent", "Starting request agent test");

		SelfInstance *pInstance = SelfInstance::GetInstance();
		Test(pInstance != NULL);

		BlackBoard *pBlackboard = pInstance->GetBlackBoard();
		Test(pBlackboard != NULL);
		VisualTeachingAgent *pVisualTeachingAgent = pInstance->FindAgent<VisualTeachingAgent>();
		Test(pVisualTeachingAgent != NULL);

		pBlackboard->SubscribeToType("Say", DELEGATE(TestVisualTeachingAgent, OnSay, const ThingEvent &, this));

		//Add a initial buffer of 10 Images on the blackboard
		std::string content = "Image_Content";
		Image::SP spImage(new Image());
		spImage->SetContent(content);
		for (int i = 0; i < 11; i++) { pBlackboard->AddThing(spImage); }

		// Add an new learning intent onto blackboard with "learn_object"
		LearningIntent::SP spLearningIntent(new LearningIntent());
		spLearningIntent->SetVerb("learn_object");
		spLearningIntent->SetTarget("water bottle");
		pBlackboard->AddThing(spLearningIntent);
		Spin( m_VisualTeachingAgentTestedCount, 1 );

		// Add an new learning intent onto blackboard with "forget_object"
		LearningIntent::SP spLearningIntentForget(new LearningIntent());
		spLearningIntentForget->SetVerb("forget_object");
		spLearningIntentForget->SetTarget("water bottle");
		pBlackboard->AddThing(spLearningIntentForget);

		// We have the count set to 2 once for learning and once for forgetting an object.
		Spin( m_VisualTeachingAgentTestedCount, 2 );
        Test( m_VisualTeachingAgentTestedCount == 2 );
		pBlackboard->UnsubscribeFromType("Say", this);
	}

	void OnSay(const ThingEvent &a_Event)
	{
		Say * pSay = DynamicCast<Say>( a_Event.GetIThing().get() );
		if (pSay->GetState() =="ADDED")
		{
			m_VisualTeachingAgentTestedCount =  m_VisualTeachingAgentTestedCount + 1;
		}
	}
};
TestVisualTeachingAgent TEST_VISUAL_TEACHING_AGENT;
