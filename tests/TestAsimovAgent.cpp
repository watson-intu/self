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
#include "agent/AsimovAgent.h"
#include "skills/SkillManager.h"
#include "skills/GestureSkill.h"
#include "skills/ISkill.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Text.h"
#include "blackboard/Goal.h"
#include "blackboard/Health.h"
#include "blackboard/Say.h"
#include "blackboard/Gesture.h"

class TestAsimovAgent : public AgentTest
{
public:
	bool			m_bAsimovAgentTested;
	bool 			m_bGoalCreated;
	int 			m_fSkillCountBegin;
	int 			m_fSkillCountEnd;

	//! Construction
	TestAsimovAgent() : AgentTest("TestAsimovAgent"),
		m_bAsimovAgentTested( false ),
		m_bGoalCreated( false ),
		m_fSkillCountBegin( 0 ),
		m_fSkillCountEnd( 0 )
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		AsimovAgent * pAsimovAgent = pInstance->FindAgent<AsimovAgent>();
		Test( pAsimovAgent != NULL );

		SkillManager * pSkillManager = SelfInstance::GetInstance()->GetSkillManager();
		pSkillManager->UseSkill("email");

		m_fSkillCountBegin = pSkillManager->GetActiveSkills().size();

		Text::SP spText( new Text("stop", .99, false, true) );
		pBlackboard->AddThing( spText );
		m_fSkillCountEnd = pSkillManager->GetActiveSkills().size();

		Test( m_fSkillCountEnd < m_fSkillCountBegin );
	}

};

TestAsimovAgent TEST_ASIMOV_AGENT;
