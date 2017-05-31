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
#include "agent/ReminderAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"
#include "blackboard/Text.h"

class TestReminderAgent : public AgentTest
{
public:
	bool			                    m_bReminderAgentTested;
	std::string                         m_ReminderUtterance;
	std::map<std::string,std::string>   m_ScheduleMap;
	std::vector<std::string>            m_Sayings;

	//! Construction
	TestReminderAgent() : AgentTest("TestReminderAgent"),
	                      m_bReminderAgentTested( false ),
	                      m_ReminderUtterance("remind me to take my Advil at twelve o'clock")
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );

		ReminderAgent * pReminderAgent = pInstance->FindAgent<ReminderAgent>();
		Test( pReminderAgent != NULL );

		m_Sayings = pReminderAgent->GetSayings();

		pBlackboard->SubscribeToType( "Say", DELEGATE(TestReminderAgent, OnSay, const ThingEvent &, this) );

		pBlackboard->AddThing(Text::SP( new Text( m_ReminderUtterance, .99, false, true ) ));

		Spin( m_bReminderAgentTested );
		Test( m_bReminderAgentTested );

		// test that the reminder is scheduled
		m_ScheduleMap = pReminderAgent->GetScheduleMap();
		Test( m_ScheduleMap.size() > 0 );

		std::map<std::string, std::string>::iterator it;
		for (it = m_ScheduleMap.begin(); it != m_ScheduleMap.end(); )
		{
			Test ( it->second == m_ReminderUtterance );
			break;
		}

		pBlackboard->UnsubscribeFromType( "Say", this );
	}

	void OnSay( const ThingEvent & a_Event )
	{
		Say * pSay = DynamicCast<Say>( a_Event.GetIThing().get() );
		Test( pSay != NULL );

		if ( (std::find(m_Sayings.begin(), m_Sayings.end(), pSay->GetText()) != m_Sayings.end()) )
		{
			Log::Debug("TestReminderAgent","Saying Found on BlackBoard: %s", pSay->GetText().c_str());
			m_bReminderAgentTested = true;
		}
	}

};

TestReminderAgent TEST_REMINDER_AGENT;
