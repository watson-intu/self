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
#include "agent/MathAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Calculate.h"
#include "blackboard/Say.h"

class TestMathAgent : public AgentTest
{
public:
	bool			m_bMathAgentTested;
	int				m_fFirstNumber;
	int 			m_fSecondNumber;

	//! Construction
	TestMathAgent() : AgentTest("TestMathAgent"),
		m_bMathAgentTested( false ),
		m_fFirstNumber( 1 ),
		m_fSecondNumber( 2 )
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		MathAgent * pMathAgent = pInstance->FindAgent<MathAgent>();
		Test( pMathAgent != NULL );

		// Convert integers to strings (to be used in data for Calculate object)
		std::stringstream ssnum1;
		ssnum1 << m_fFirstNumber;
		std::string num1 = ssnum1.str();

		std::stringstream ssnum2;
		ssnum2 << m_fSecondNumber;
		std::string num2 = ssnum2.str();


		pBlackboard->SubscribeToType( "Calculate", DELEGATE(TestMathAgent, OnCalculate, const ThingEvent &, this) );

		// Create string of json array data (to be used by Calculate object)
		std::string data = "[{\"number\":" + num1 + "},{\"number\":" + num2 + "}]";
		Calculate::SP spCalculate( new Calculate(data, "SUM", "number") );
		pBlackboard->AddThing( spCalculate );
		Spin( m_bMathAgentTested );
		Test( m_bMathAgentTested );

		pBlackboard->UnsubscribeFromType( "Calculate", this );
	}

	void OnCalculate( const ThingEvent & a_Event )
	{
		Calculate * pCalculate = DynamicCast<Calculate>( a_Event.GetIThing().get() );
		Test( pCalculate != NULL );

		Calculate::ThingList childrenList = pCalculate->GetChildren();
		Test( childrenList.size() > 0 );

		Say::SP spSay = DynamicCast<Say>( childrenList.front() );
		Test( spSay != NULL );
		const std::string & text = spSay->GetText();
		Test( !text.empty() );

		// Test that the Say object text is indeed the summation of the two numbers
		Test( atoi(text.c_str()) == (m_fFirstNumber + m_fSecondNumber) );

		if ( pCalculate->GetState() == "COMPLETED" )
			m_bMathAgentTested = true;
	}
};

TestMathAgent TEST_MATH_AGENT;
