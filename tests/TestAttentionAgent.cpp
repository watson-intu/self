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
#include "agent/AttentionAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Attention.h"

class TestAttentionAgent: public AgentTest
{
public:
	bool			m_bAttentionAgentTested;

	//! Construction
	TestAttentionAgent() : AgentTest("TestAttentionAgent"),
		m_bAttentionAgentTested( false )
	{}

	virtual void RunTest()
	{
		Log::Status("TestAttentionAgent", "Starting Attention test");

		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		AttentionAgent * pAttentionAgent = pInstance->FindAgent<AttentionAgent>();
		Test( pAttentionAgent != NULL );

		// Post proximity object on blackboard
		pBlackboard->SubscribeToType( "Attention", DELEGATE(TestAttentionAgent, OnAttention, const ThingEvent &, this) );
		Proximity::SP spProximity( new Proximity() );
		spProximity->SetDistance(0.1f);
		pBlackboard->AddThing( spProximity );

		// Check Attention Agent reaches the completed status
		Spin( m_bAttentionAgentTested);
		Test( m_bAttentionAgentTested );

		pBlackboard->UnsubscribeFromType( "Attention", this );
	}

	//Callback to see if AttentionAgent reaches completed state.
	void OnAttention( const ThingEvent & a_Event )
	{
		Attention * pAttention= DynamicCast<Attention>( a_Event.GetIThing().get() );
		Test( pAttention!= NULL );
		if ( pAttention != NULL){
			m_bAttentionAgentTested= true;
		}
	}
};

TestAttentionAgent TEST_ATTENTION_AGENT;
