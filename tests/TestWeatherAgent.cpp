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
#include "agent/WeatherAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Text.h"
#include "blackboard/Goal.h"

class TestWeatherAgent : public AgentTest
{
public:
	bool			m_bWeatherAgentTested;

	//! Construction
	TestWeatherAgent() : AgentTest("TestWeatherAgent"),
		m_bWeatherAgentTested( false )
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		WeatherAgent * pWeatherAgent = pInstance->FindAgent<WeatherAgent>();
		Test( pWeatherAgent != NULL );

		pBlackboard->SubscribeToType( "Goal", DELEGATE(TestWeatherAgent, OnWeatherRequest, const ThingEvent &, this) );

		Text::SP spText( new Text("What is the weather in Phoenix", .99, false, true) );
		pBlackboard->AddThing( spText );
		Spin( m_bWeatherAgentTested );
		Test( m_bWeatherAgentTested );

		pBlackboard->UnsubscribeFromType( "Goal", this );

	}

	void OnWeatherRequest( const ThingEvent & a_Event )
	{
		Goal * pGoal = DynamicCast<Goal>( a_Event.GetIThing().get() );
		Test( pGoal != NULL );

		if ( pGoal->GetName() == "Weather" )
		{
			Test(pGoal->GetParams().GetData().isMember("temperature"));
			if(pGoal->GetState() == "COMPLETED")
				m_bWeatherAgentTested = true;
		}
	}

};

TestWeatherAgent TEST_WEATHER_AGENT;
