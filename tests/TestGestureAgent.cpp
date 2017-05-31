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
#include "agent/GestureAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"

class TestGestureAgent : public AgentTest
{
public:
	bool			m_bGestureAgentTested;

	//! Construction
	TestGestureAgent() : AgentTest("TestGestureAgent"),
		m_bGestureAgentTested(false)
	{}

	virtual void RunTest()
	{
		Log::Status("TestGestureAgent", "Starting gesture test");

		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test(pInstance != NULL);
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test(pBlackboard != NULL);
		GestureAgent * pGestureAgent = pInstance->FindAgent<GestureAgent>();
		Test(pGestureAgent != NULL);

		// Post display object on blackboard
		pBlackboard->SubscribeToType("Gesture", DELEGATE(TestGestureAgent, OnGesture, const ThingEvent &, this));
		ParamsMap params;
		params["text"] = "This is a test of the gesture agent";
		Gesture::SP spGesture(new Gesture("tts", params));
		pBlackboard->AddThing(spGesture);

		// Check gesture agent reaches the completed status
		Spin(m_bGestureAgentTested);
		Test(m_bGestureAgentTested);

		pBlackboard->UnsubscribeFromType("Gesture", this);
	}

	//Callback to see if GestureAgent reaches completed state.
	void OnGesture(const ThingEvent & a_Event)
	{
		Gesture * pGesture = DynamicCast<Gesture>(a_Event.GetIThing().get());
		Test(pGesture != NULL);

		if (pGesture->GetState() == "COMPLETED")
			m_bGestureAgentTested = true;
	}
};

TestGestureAgent TEST_GESTURE_AGENT;
