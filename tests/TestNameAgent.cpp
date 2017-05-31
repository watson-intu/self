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
#include "agent/NameAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/NameIntent.h"
#include "blackboard/Text.h"

class TestNameAgent : public AgentTest
{
public:
	bool			m_bNameAgentTested;
	std::string		m_originalName;
	std::string		m_newName;

	//! Construction
	TestNameAgent() : AgentTest("TestNameAgent"),
		m_bNameAgentTested( false ),
		m_originalName(""),
		m_newName("")
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		NameAgent * pNameAgent = pInstance->FindAgent<NameAgent>();
		Test( pNameAgent != NULL );

		m_originalName = pInstance->GetLocalConfig().m_Name;
		Test( !m_originalName.empty() );

		pBlackboard->SubscribeToType( "NameIntent", DELEGATE(TestNameAgent, OnLearnNewName, const ThingEvent &, this) );
		Text::SP spText( new Text( "Your name is now Maximilian", .99, false, true) );
		pBlackboard->AddThing( spText );
		Spin( m_bNameAgentTested );
		Test( m_bNameAgentTested );

		m_newName = pInstance->GetLocalConfig().m_Name;
		Test( m_newName != m_originalName );

		pBlackboard->UnsubscribeFromType( "NameIntent", this );
	}

	void OnLearnNewName( const ThingEvent & a_Event )
	{
		NameIntent * pNameIntent = DynamicCast<NameIntent>( a_Event.GetIThing().get() );
		Test( pNameIntent != NULL );
		Test( !pNameIntent->GetName().empty() );

		if ( pNameIntent->GetState() == "COMPLETED" )
			m_bNameAgentTested = true;
	}
};

TestNameAgent TEST_NAME_AGENT;
