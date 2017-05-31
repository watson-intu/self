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


#include "WeatherAgent.h"
#include "blackboard/Goal.h"
#include "blackboard/Environment.h"
#include "blackboard/WeatherIntent.h"
#include "blackboard/Say.h"
#include "blackboard/Person.h"

REG_SERIALIZABLE(WeatherAgent);
RTTI_IMPL(WeatherAgent, IAgent);

void WeatherAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);

	json["m_bFahrenheit"] = m_bFahrenheit;
	json["m_DefaultCity"] = m_DefaultCity;
	json["m_WeatherOutOfRangeMessage"] = m_WeatherOutOfRangeMessage;
}

void WeatherAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);

	if (json.isMember("m_bFahrenheit"))
		m_bFahrenheit = json["m_bFahrenheit"].asBool();
	if (json.isMember("m_WeatherOutOfRangeMessage"))
		m_WeatherOutOfRangeMessage = json["m_WeatherOutOfRangeMessage"].asString();
	if (json.isMember("m_DefaultCity"))
		m_DefaultCity = json["m_DefaultCity"].asString();
}

bool WeatherAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	IWeather * pWeather = pInstance->FindService<IWeather>();
	if (pWeather == NULL)
	{
		Log::Error("WeatherAgent", "Failed to find IWeather service.");
		return false;
	}
	ILocation * pLocation = pInstance->FindService<ILocation>();
	if (pLocation == NULL)
	{
		Log::Error("WeatherAgent", "Failed to find ILocation service.");
		return false;
	}

	pInstance->GetBlackBoard()->SubscribeToType("Environment",
		DELEGATE(WeatherAgent, OnAnomaly, const ThingEvent &, this), TE_STATE);
	pInstance->GetBlackBoard()->SubscribeToType("WeatherIntent",
		DELEGATE(WeatherAgent, OnWeatherRequest, const ThingEvent &, this), TE_ADDED);

	return true;
}

bool WeatherAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->UnsubscribeFromType("Environment", this);
	pInstance->GetBlackBoard()->UnsubscribeFromType("WeatherIntent", this);
	return true;
}

void WeatherAgent::OnAnomaly(const ThingEvent &a_ThingEvent)
{
	Environment::SP spEnvironment = DynamicCast<Environment>(a_ThingEvent.GetIThing());

	if (StringUtil::StartsWith(spEnvironment->GetState(), "Anomaly"))
	{
		Goal::SP spGoal(new Goal(spEnvironment->GetState(), spEnvironment->ToJson()));
		Log::Debug("WeatherAgent", "Adding Anomaly goal: %s", spGoal->ToJson().toStyledString().c_str());
		spEnvironment->AddChild(spGoal);
	}
}

void WeatherAgent::OnWeatherRequest(const ThingEvent & a_ThingEvent)
{
	WeatherIntent::SP spWeather = DynamicCast<WeatherIntent>(a_ThingEvent.GetIThing());
	if (spWeather)
		new WeatherRequest(this, spWeather);
}

WeatherAgent::WeatherRequest::WeatherRequest(WeatherAgent * a_pAgent, const WeatherIntent::SP & a_spIntent) : m_pAgent(a_pAgent), m_spIntent(a_spIntent)
{
	m_DateSpecified = m_spIntent->GetDate();
	if (m_spIntent->GetLocation().empty() && !m_pAgent->m_DefaultCity.empty())
		m_spIntent->SetLocation(m_pAgent->m_DefaultCity);

	ILocation * pLocation = Config::Instance()->FindService<ILocation>();
	if (pLocation != NULL)
	{
		m_City = m_spIntent->GetLocation();
		pLocation->GetLocation(m_City, DELEGATE(WeatherRequest, OnLocation, const Json::Value &, this));
	}
	else
		delete this;
}


void WeatherAgent::WeatherRequest::OnLocation(const Json::Value & json)
{
	IWeather * pWeather = Config::Instance()->FindService<IWeather>();
	if (pWeather != NULL)
	{
		bool bLocationFound = false;
		if (json.isMember("location"))
		{
			Json::Value location = json["location"];
			Log::Debug("WeatherAgent", "Location: %s", location.toStyledString().c_str());
			if (location.isMember("latitude"))
			{
				float fLat = 0.0f;
				float fLong = 0.0f;

				if (location["latitude"].isArray())
				{
					fLat = location["latitude"][0].asFloat();
					fLong = location["longitude"][0].asFloat();
				}
				else
				{
					fLat = location["latitude"].asFloat();
					fLong = location["longitude"].asFloat();
				}

				Location a_Location(m_City, fLat, fLong);

				if (m_DateSpecified.size() > 0)
				{
					pWeather->GetTenDayForecast(&a_Location,
						DELEGATE(WeatherRequest, OnTenDayForecastReceived, const Json::Value &, this));
				}
				else
				{
					pWeather->GetCurrentConditions(&a_Location,
						DELEGATE(WeatherRequest,
							OnCurrentConditionsReceived, const Json::Value &, this));
				}

				bLocationFound = true;
			}
		}

		if (!bLocationFound)
		{
			if (m_DateSpecified.empty())
			{
				pWeather->GetCurrentConditions(NULL,
					DELEGATE(WeatherRequest,
						OnCurrentConditionsReceived,
						const Json::Value &, this));
			}
			else
			{
				pWeather->GetTenDayForecast(NULL,
					DELEGATE(WeatherRequest,
						OnTenDayForecastReceived, const Json::Value &, this));
			}
		}
	}
	else
		delete this;
}

void WeatherAgent::WeatherRequest::OnCurrentConditionsReceived(const Json::Value & json)
{
	if (json["forecasts"].isArray() && json["forecasts"].size() > 0)
	{
		Json::Value forecasts = json["forecasts"][0];

		float temperature = forecasts["temp"].asFloat();
		if (m_pAgent->m_bFahrenheit)
			IWeather::CelsiusToFahrenheit(temperature, temperature);

		int roundTemp = (int)floor(temperature);

		std::string skyCover;
		if (forecasts.isMember("phrase_32char"))
			skyCover = forecasts["phrase_32char"].asString();

		Goal::SP spGoal(new Goal("Weather"));
		spGoal->GetParams()["sky_cover"] = skyCover;
		spGoal->GetParams()["temperature"] = roundTemp;
		spGoal->GetParams()["location"] = m_City;
		spGoal->GetParams()["date"] = "currently";

		m_spIntent->AddChild(spGoal);
	}

	delete this;
}

void WeatherAgent::WeatherRequest::OnTenDayForecastReceived(const Json::Value & json)
{
	float temperature = 0.0f;
	std::string skyCover;

	if (json.isMember("forecasts"))
	{
		const Json::Value & forecasts = json["forecasts"];
		for (size_t i = 0; i < forecasts.size(); ++i)
		{
			if (forecasts[i].isMember("fcst_valid_local") &&
				StringUtil::StartsWith(forecasts[i]["fcst_valid_local"].asString(), m_DateSpecified))
			{
				if (forecasts[i].isMember("day"))
				{
					Json::Value dayForecast = forecasts[i]["day"];
					skyCover = dayForecast["shortcast"].asString();
					if (m_pAgent->m_bFahrenheit)
						IWeather::CelsiusToFahrenheit(dayForecast["temp"].asFloat(), temperature);
					else
						temperature = dayForecast["temp"].asFloat();
				}
				break;
			}
		}
	}

	int roundTemp = (int)floor(temperature);
	if (!skyCover.empty())
	{
		Goal::SP spGoal(new Goal("Weather"));
		spGoal->GetParams()["sky_cover"] = skyCover;
		spGoal->GetParams()["temperature"] = roundTemp;
		spGoal->GetParams()["location"] = m_City;
		spGoal->GetParams()["date"] = "for that day";

		m_spIntent->AddChild(spGoal);
	}
	else
	{
		Say::SP spSay(new Say(m_pAgent->m_WeatherOutOfRangeMessage));
		m_spIntent->AddChild(spSay);
	}

	delete this;
}

