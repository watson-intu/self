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
#include "agent/SkillTeachingAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Text.h"
#include "blackboard/LearningIntent.h"
#include "blackboard/Goal.h"
#include "blackboard/Gesture.h"

class TestSkillTeachingAgent : public AgentTest
{
public:
	bool			m_bSkillForgettenTested;
	bool 			m_bFailedGoalTested;
	bool 			m_bSkillLearnedTested;
	std::string		m_ActionWord;

	//! Construction
	TestSkillTeachingAgent() : AgentTest("TestSkillTeachingAgent"),
		m_bSkillForgettenTested( false ),
		m_bFailedGoalTested( false ),
		m_bSkillLearnedTested( false ),
		m_ActionWord("revolt")
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		SkillTeachingAgent * pSkillTeachingAgent = pInstance->FindAgent<SkillTeachingAgent>();
		Test( pSkillTeachingAgent != NULL );

		pBlackboard->SubscribeToType( "LearningIntent", DELEGATE(TestSkillTeachingAgent, OnLearningIntent, const ThingEvent &, this) );
		pBlackboard->SubscribeToType( "Goal", DELEGATE(TestSkillTeachingAgent, OnGoal, const ThingEvent &, this) );
		Text::SP spText1( new Text(m_ActionWord + " with the crowd", .99, false, true) );
		Text::SP spText2( new Text("that is how you " + m_ActionWord, .99, false, true) );
		Text::SP spText3( new Text("forget how to " + m_ActionWord, .99, false, true) );

		// Add Text that will cause a goal failed state to the blackboard
		// Wait for the failed goal to be recognized before continuing on
		pBlackboard->AddThing( spText1 );
		Spin( m_bFailedGoalTested );
		Test( m_bFailedGoalTested );

		// Populate skill list
		Gesture::SP spGesture( new Gesture("volume_default") );
		pBlackboard->AddThing(spGesture);

		// Add text that will trigger learn_skill
		// Wait for the skill to be learned before continuing on
		pBlackboard->AddThing( spText2 );
		Spin( m_bSkillLearnedTested );
		Test( m_bSkillLearnedTested );

		// Add text that will trigger forget_skill
		pBlackboard->AddThing( spText3 );
		Spin( m_bSkillForgettenTested );
		Test( m_bSkillForgettenTested );

		pBlackboard->UnsubscribeFromType( "LearningIntent", this );
		pBlackboard->UnsubscribeFromType( "Goal", this );
	}

	void OnLearningIntent( const ThingEvent & a_Event )
	{
		LearningIntent * pLearningIntent = DynamicCast<LearningIntent>( a_Event.GetIThing().get() );
		Test( pLearningIntent != NULL );

		if( pLearningIntent->GetVerb() == "learn_skill" && pLearningIntent->GetState() == "COMPLETED" )
			m_bSkillLearnedTested = true;

		if( pLearningIntent->GetVerb() == "forget_skill"  && pLearningIntent->GetState() == "COMPLETED" )
			m_bSkillForgettenTested = true;
	}

	void OnGoal( const ThingEvent & a_Event )
	{
		Goal * pGoal = DynamicCast<Goal>( a_Event.GetIThing().get() );
		Test( pGoal != NULL );

		if( pGoal->GetState() == "FAILED" && pGoal->GetName() == m_ActionWord )
			m_bFailedGoalTested = true;
	}
};

TestSkillTeachingAgent TEST_SKILL_TEACHING_AGENT;
