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


#include "ReminderAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Goal.h"
#include "blackboard/Say.h"
#include "SelfInstance.h"

REG_SERIALIZABLE(ReminderAgent);
RTTI_IMPL(ReminderAgent, IAgent);

void ReminderAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
	json["m_Delay"] = m_Delay;
	SerializeVector("m_Sayings", m_Sayings, json);
	SerializeMap("m_ScheduleMap", m_ScheduleMap, json);

}

void ReminderAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
	m_Delay = json["m_Delay"].asFloat();
	DeserializeVector("m_Sayings", json, m_Sayings);
	if (m_Sayings.size() == 0)
		m_Sayings.push_back("Not a problem");
	DeserializeMap("m_ScheduleMap", json, m_ScheduleMap);
}

bool ReminderAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->SubscribeToType("LearningIntent",
		DELEGATE(ReminderAgent, OnLearnedIntent, const ThingEvent &, this), TE_ADDED);

	m_spScheduleTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(ReminderAgent, OnCheckSchedule, this),
		m_Delay, true, true);
	return true;
}

bool ReminderAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->UnsubscribeFromType("LearningIntent", this);
	m_spScheduleTimer.reset();
	return true;
}

void ReminderAgent::OnLearnedIntent(const ThingEvent & a_ThingEvent)
{
	LearningIntent::SP spIntent = DynamicCast<LearningIntent>(a_ThingEvent.GetIThing());
	if (spIntent->GetVerb() == "learn_schedule")
		OnAddReminder(spIntent);
}

void ReminderAgent::OnAddReminder(LearningIntent::SP a_spIntent)
{
	std::string a_Time;
	StringUtil::ConvertToTime(a_spIntent->GetTarget(), a_Time);
	double epochTime = Time(Time::ParseTime(a_Time)).GetEpochTime();
	double currentTime = Time().GetEpochTime();
	Log::Debug("ReminderAgent", "Adding new reminder at %f, current time is (%f). Difference is: %f", epochTime, currentTime, epochTime - currentTime);

	m_ScheduleMap[StringUtil::Format("%f", epochTime)] = a_spIntent->GetText();
	std::string message = m_Sayings[rand() % m_Sayings.size()];
	SelfInstance::GetInstance()->GetBlackBoard()->AddThing(Say::SP(new Say(message)));
}

void ReminderAgent::OnCheckSchedule()
{
	double currentTime = Time().GetEpochTime();
	std::map<std::string, std::string>::iterator it;
	for (it = m_ScheduleMap.begin(); it != m_ScheduleMap.end(); )
	{
		if (currentTime > atof(it->first.c_str()))
		{
			std::string message = "Remember to " + it->second;
			Goal::SP spGoal(new Goal("Reminder"));
			spGoal->GetParams()["reminder"]["text"] = message;
			SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spGoal);
			m_ScheduleMap.erase(it++);
			Log::Debug("ReminderAgent", "Added Goal to Blackboard!");
		}
		else
			++it;
	}
}




