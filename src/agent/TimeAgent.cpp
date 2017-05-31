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


#define _CRT_SECURE_NO_WARNINGS

#include "TimeAgent.h"
#include "blackboard/Goal.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Text.h"
#include "services/ILocation.h"
#include "SelfInstance.h"

REG_SERIALIZABLE(TimeAgent);
RTTI_IMPL(TimeAgent, IAgent);

void TimeAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
	json["m_TimeZone"] = m_TimeZone;
	json["m_TimeZoneDB"] = m_TimeZoneDB;
	json["m_TimeTag"] = m_TimeTag;
}

void TimeAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
	if (json.isMember("m_TimeZone"))
		m_TimeZone = json["m_TimeZone"].asString();
	if (json.isMember("m_TimeZoneDB"))
		m_TimeZoneDB = json["m_TimeZoneDB"].asString();
	if (json.isMember("m_TimeTag"))
		m_TimeTag = json["m_TimeTag"].asString();
}

bool TimeAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->SubscribeToType("TimeIntent",
		DELEGATE(TimeAgent, OnTimeIntent, const ThingEvent &, this), TE_ADDED);

	try {
		m_TZdb.load_from_file(Config::Instance()->GetStaticDataPath() + m_TimeZoneDB);
	}
	catch (const std::exception & ex)
	{
		Log::Error("TimeAgent", "Caught Exception: %s:Failed to load TimeZone DB", ex.what());
	}

	return true;
}

bool TimeAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->UnsubscribeFromType("TimeIntent", this);
	return true;
}

void TimeAgent::OnTimeIntent(const ThingEvent & a_ThingEvent)
{
	bool bHaveLocation = true;

	TimeIntent::SP spTime = DynamicCast<TimeIntent>(a_ThingEvent.GetIThing());
	if (spTime)
	{
		m_spActive = spTime;

		ILocation * pLocation = Config::Instance()->FindService<ILocation>();
		if (pLocation != NULL && !spTime->GetLocation().empty())
		{
			boost::shared_ptr<TimeAgent> spThis(boost::static_pointer_cast<TimeAgent>(shared_from_this()));
			pLocation->GetLocation(spTime->GetLocation(), DELEGATE(TimeAgent, OnTime, const Json::Value &, spThis));
			bHaveLocation = false;
		}
	}

	if (bHaveLocation)
		OnExecute(m_TimeZone);
}

void TimeAgent::OnTime(const Json::Value & json)
{
	bool bFailed = true;
	if (json.isMember("location"))
	{
		Json::Value location = json["location"];
		if (location.isMember("latitude") && location.isMember("longitude"))
		{
			Json::Value latitude = location["latitude"];
			Json::Value longitude = location["longitude"];
			if (latitude.isArray()
				&& latitude.size() > 0
				&& longitude.isArray()
				&& longitude.size() > 0)
			{
				ILocation * pLocation = Config::Instance()->FindService<ILocation>();
				if (pLocation != NULL)
				{
					boost::shared_ptr<TimeAgent> spThis(boost::static_pointer_cast<TimeAgent>(shared_from_this()));
					pLocation->GetTimeZone(latitude[0].asDouble(), longitude[0].asDouble(),
						DELEGATE(TimeAgent, OnTimeZone, const Json::Value &, spThis));
					bFailed = false;
				}
			}
		}
	}

	if (bFailed)
		OnExecute(m_TimeZone);
}

void TimeAgent::OnTimeZone(const Json::Value & json)
{
	// Do not override m_TimeZone -- it stores the default timezone in case a location is not specified
	std::string a_TimeZone = m_TimeZone;
	if (json.isMember("location"))
	{
		Json::Value location = json["location"];

		if (location.isMember("ianaTimeZone"))
		{
			if (location["ianaTimeZone"].isArray())
				a_TimeZone = location["ianaTimeZone"][0].asString();
			else
				a_TimeZone = location["ianaTimeZone"].asString();
		}
	}

	OnExecute(a_TimeZone);
}

void TimeAgent::OnExecute(const std::string & a_TimeZone)
{
	boost::local_time::time_zone_ptr some_tz = m_TZdb.time_zone_from_region(a_TimeZone);
	bool inDST = false;
	if (!some_tz)
	{
		Log::Error("TimeAgent", "Cannot find time zone in db!");
		Goal::SP spGoal(new Goal("Time"));
		m_spActive->AddChild(spGoal);
		return;
	}

	// UTC Offset
	boost::posix_time::time_duration utc_offset = some_tz->base_utc_offset();
	int utc_offset_hours = utc_offset.hours();
	int utc_offset_mins = utc_offset.minutes();

	// Daylight Savings Offset
	boost::posix_time::time_duration dst_offset = some_tz->dst_offset();
	int dst_offset_hours = dst_offset.hours();

	Log::Debug("TimeAgent", "UTC Offset: %d : %d | DST Offset: %d", utc_offset_hours, utc_offset_mins, dst_offset_hours);
	time_t now = time(0);
	struct tm gmt = *gmtime(&now);

	// If a timezone is effected by DST, check to see if the timezone is currently in DST
	if (dst_offset_hours > 0)
	{
		// Get Daylight Savings start and end dates
		boost::posix_time::ptime::time_type now_time = boost::posix_time::from_time_t(now);
		boost::posix_time::ptime::time_type dst_start_time = some_tz->dst_local_start_time(gmt.tm_year + 1900);
		boost::posix_time::ptime::time_type dst_end_time = some_tz->dst_local_end_time(gmt.tm_year + 1900);

		// Check to see if the timezone is currently in Daylight Savings...
		// If the current time is after DST start but before DST end
		if (now_time > dst_start_time && dst_end_time > now_time)
			inDST = true;
	}

	int hour = gmt.tm_hour + utc_offset_hours;
	if (inDST)
		hour += dst_offset_hours;

	// Need to take into consideration that some UTC offsets include minute offsets as well
	// Example: Darwin, Australia is +09:30:00
	int minute = gmt.tm_min + utc_offset_mins;
	if (minute >= 60)
	{
		hour += 1;
		minute -= 60;
	}

	std::string meridium = "AM";
	if (hour > 23)
		hour = hour - 24;
	else if (hour > 11)
	{
		hour = hour - 12;
		meridium = "PM";
	}
	if (hour == 0)
		hour = 12;

	std::string time;
	if (minute < 10)
		time = StringUtil::Format("%d:0%d", hour, minute);
	else
		time = StringUtil::Format("%d:%d", hour, minute);

	Goal::SP spGoal(new Goal("Time"));
	spGoal->GetParams()["meridium"] = meridium;
	spGoal->GetParams()["time"] = time;
	if (m_spActive->GetLocation().size() > 0)
		spGoal->GetParams()["city"] = m_spActive->GetLocation();
	m_spActive->AddChild(spGoal);
}

