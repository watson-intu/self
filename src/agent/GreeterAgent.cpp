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


#include "GreeterAgent.h"
#include "utils/TimerPool.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/IThing.h"
#include "blackboard/Goal.h"
#include "blackboard/Say.h"
#include "classifiers/FaceClassifier.h"
#include "utils/Time.h"
#include "skills/ISkill.h"

REG_SERIALIZABLE(GreeterAgent);
RTTI_IMPL(GreeterAgent, IAgent);

const std::string NEW_FACE("_new_face");

GreeterAgent::GreeterAgent() :
	m_GreetTimeout(60.0f),
	m_SeenTimeout(120.0f),
	m_GreetingIntent("greeting_interaction")
{}

void GreeterAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);

	json["m_GreetTimeout"] = m_GreetTimeout;
	json["m_SeenTimeout"] = m_SeenTimeout;
	json["m_GreetingIntent"] = m_GreetingIntent;
}

void GreeterAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);

	if (json["m_GreetTimeout"].isDouble())
		m_GreetTimeout = json["m_GreetTimeout"].asFloat();
	if (json["m_SeenTimeout"].isDouble())
		m_SeenTimeout = json["m_SeenTimeout"].asFloat();
	if (json["m_GreetingIntent"].isString())
		m_GreetingIntent = json["m_GreetingIntent"].asString();
}

bool GreeterAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert(pInstance != NULL);

	FaceClassifier * pFaceClassifier = pInstance->FindClassifier<FaceClassifier>();
	if (pFaceClassifier != NULL)
	{
		pInstance->GetBlackBoard()->SubscribeToType("RecognizedFace",
			DELEGATE(GreeterAgent, OnRecognizedFace, const ThingEvent &, this), TE_ADDED);
		pInstance->GetBlackBoard()->SubscribeToType("NewFace",
			DELEGATE(GreeterAgent, OnNewFace, const ThingEvent &, this), TE_ADDED);
	}
	else
	{
		pInstance->GetBlackBoard()->SubscribeToType("Person",
			DELEGATE(GreeterAgent, OnPerson, const ThingEvent &, this), TE_ADDED);
	}

	pInstance->GetBlackBoard()->SubscribeToType("Say",
		DELEGATE(GreeterAgent, OnSay, const ThingEvent &, this), TE_ADDED);

	return true;
}

bool GreeterAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert(pInstance != NULL);

	pInstance->GetBlackBoard()->UnsubscribeFromType("RecognizedFace", this);
	pInstance->GetBlackBoard()->UnsubscribeFromType("NewFace", this);
	pInstance->GetBlackBoard()->UnsubscribeFromType("Say", this);
	pInstance->GetBlackBoard()->UnsubscribeFromType("Person", this);

	return true;
}

void GreeterAgent::OnPerson(const ThingEvent & a_ThingEvent)
{
	Person::SP spPerson = DynamicCast<Person>(a_ThingEvent.GetIThing());
	if (spPerson)
	{
		double fLastGreeting = Time().GetEpochTime() - m_LastGreeting[NEW_FACE];
		if (fLastGreeting > m_GreetTimeout)
		{
			Json::Value context;
			context["m_PersonGender"] = spPerson->GetGender();
			context["m_PersonAge"] = spPerson->GetAge();
			context["m_PersonName"] = "";
			context["m_SensorName"] = spPerson->GetOrigin();

			ISensor::SP spSensor = SelfInstance::GetInstance()->GetSensorManager()->FindSensor(spPerson->GetOrigin());
			if (spSensor)
				context["m_SensorName"] = spSensor->GetSensorName();

			Goal::SP spGoal(new Goal("Greeter"));
			spGoal->SetParams(context);
			spPerson->AddChild(spGoal);

			m_LastGreeting[NEW_FACE] = Time().GetEpochTime();
		}

	}
}

void GreeterAgent::OnNewFace(const ThingEvent & a_ThingEvent)
{
	IThing::SP spNewFace = a_ThingEvent.GetIThing();
	Person::SP spPerson = spNewFace->FindParentType<Person>(true);
	if (spPerson)
	{
		double fLastGreeting = Time().GetEpochTime() - m_LastGreeting[NEW_FACE];
		if (fLastGreeting > m_GreetTimeout)
		{
			Json::Value context;
			context["m_PersonGender"] = spPerson->GetGender();
			context["m_PersonAge"] = spPerson->GetAge();
			context["m_PersonName"] = "";
			context["m_SensorName"] = spPerson->GetOrigin();

			ISensor::SP spSensor = SelfInstance::GetInstance()->GetSensorManager()->FindSensor(spPerson->GetOrigin());
			if (spSensor)
				context["m_SensorName"] = spSensor->GetSensorName();

			Goal::SP spGoal(new Goal("Greeter"));
			spGoal->SetParams(context);
			spPerson->AddChild(spGoal);

			m_LastGreeting[NEW_FACE] = Time().GetEpochTime();
		}
	}
}

void GreeterAgent::OnRecognizedFace(const ThingEvent & a_ThingEvent)
{
	IThing::SP spRecognizedFace = a_ThingEvent.GetIThing();

	std::string personId = (*spRecognizedFace)["personId"].asString();
	Person::SP spPerson = a_ThingEvent.GetIThing()->FindParentType<Person>(true);
	if (spPerson)
	{
		Image::SP spImage = spPerson->FindParentType<Image>(true);
		if (spImage)
		{
			std::string key = spImage->GetOrigin() + "/" + personId;
			if (m_LastGreeting.find(key) == m_LastGreeting.end() || (Time().GetEpochTime() - m_LastGreeting[key]) > m_SeenTimeout)
			{
				Json::Value context;
				context["m_PersonGender"] = spPerson->GetGender();
				context["m_PersonAge"] = spPerson->GetAge();
				context["m_PersonName"] = (*spRecognizedFace)["fullName"];

				context["m_SensorId"] = spImage->GetOrigin();

				ISensor::SP spSensor = SelfInstance::GetInstance()->GetSensorManager()->FindSensor(spImage->GetOrigin());
				if (spSensor)
					context["m_SensorName"] = spSensor->GetSensorName();

				Goal::SP spGoal(new Goal("Greeter"));
				spGoal->SetParams(context);
				spRecognizedFace->AddChild(spGoal);
			}

			// we MUST keep updating the last time otherwise these greetings keep firing off while the person is visible!
			m_LastGreeting[key] = Time().GetEpochTime();
		}
	}
}

void GreeterAgent::OnSay(const ThingEvent & a_ThingEvent)
{
	m_LastGreeting[NEW_FACE] = Time().GetEpochTime();
}
