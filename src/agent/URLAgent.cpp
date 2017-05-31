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


#include "URLAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"
#include "blackboard/Goal.h"
#include "utils/IService.h"
#include "SelfLib.h"

REG_SERIALIZABLE(URLAgent);
RTTI_IMPL(URLAgent, IAgent);

void URLAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);

	SerializeVector("m_FailureResponses", m_FailureResponses, json);
}

void URLAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);

	DeserializeVector("m_FailureResponses", json, m_FailureResponses);
}

bool URLAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->SubscribeToType("Url",
		DELEGATE(URLAgent, OnURL, const ThingEvent &, this), TE_ADDED);

	return true;
}

bool URLAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->UnsubscribeFromType("Url", this);
	return true;
}

void URLAgent::OnURL(const ThingEvent & a_ThingEvent)
{
	Url::SP spUrl = DynamicCast<Url>(a_ThingEvent.GetIThing());
	if (spUrl)
	{
		// Make a call to a service
		IBrowser * pService = SelfInstance::GetInstance()->FindService<IBrowser>();
		if (pService != NULL)
		{
			pService->ShowURL(spUrl,
				DELEGATE(URLAgent, OnResponse, IBrowser::URLServiceData *, this));
		}
		else
		{
			Log::Error("URLAgent", "No URLService found.");
			spUrl->SetState("FAILED");
		}

		if (pService != NULL)
		{
			// Create a Goal 
			Json::Value context;
			context["m_Message"] = spUrl->GetURL();

			Goal::SP spGoal(new Goal("URLDisplay"));
			spGoal->SetParams(context);

			SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spGoal);
		}
	}
}


void URLAgent::OnResponse(IBrowser::URLServiceData * a_UrlAgentData)
{
	if (a_UrlAgentData == NULL || a_UrlAgentData->m_JsonValue.isNull())
	{
		a_UrlAgentData->m_spUrl->SetState("FAILED");
		if (m_FailureResponses.size() > 0)
		{
			SelfInstance::GetInstance()->GetBlackBoard()->AddThing(
				Say::SP(new Say(m_FailureResponses[rand() % m_FailureResponses.size()])));
		}
	}
	else
	{
		a_UrlAgentData->m_spUrl->SetState("COMPLETED");
		Log::Status("URLAgent", "The JSON returned was all right");
	}

	delete a_UrlAgentData;
}

