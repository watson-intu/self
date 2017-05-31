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
#include "agent/MusicAgent.h"
#include "blackboard/UsedSkill.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/IThing.h"
#include <string.h>

class TestMusicAgent : public AgentTest
{
public:
	bool			m_bMusicAgentTested;
	Json::Value data;
	SelfInstance * pInstance;

	//! Construction
	TestMusicAgent() : AgentTest("TestMusicAgent"),
		m_bMusicAgentTested(false)
	{}

	virtual void RunTest()
	{
		data["BPM"] = 1000;
		pInstance = SelfInstance::GetInstance();
		Test(pInstance != NULL);
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test(pBlackboard != NULL);
		MusicAgent * pMusicAgent = pInstance->FindAgent<MusicAgent>();
		Test(pMusicAgent != NULL);
		

		pBlackboard->SubscribeToType("UsedSkill", DELEGATE(TestMusicAgent, OnBeat, const ThingEvent &, this) );
		pBlackboard->AddThing(IThing::SP(new IThing(TT_PERCEPTION, "MusicStarted",data, 60.0f)));
		pBlackboard->AddThing(IThing::SP(new IThing( TT_PERCEPTION,"MusicBeat", data, 60.0f)));
		Spin(m_bMusicAgentTested);
		Test(m_bMusicAgentTested);

		pBlackboard->UnsubscribeFromType("UsedSkill", this);
	}

	void OnBeat(const ThingEvent & a_Event)
	{
		UsedSkill * pUsedSkill = DynamicCast<UsedSkill>(a_Event.GetIThing().get());
		Test( pUsedSkill->GetSkill() != NULL);
		if (strcmp(pUsedSkill->GetSkill()->GetSkillName().c_str(), "dance" ) == 0)
			m_bMusicAgentTested = true;
	}
};

TestMusicAgent TEST_MUSIC_AGENT;
