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


#include "WebRequestAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Goal.h"

REG_SERIALIZABLE(WebRequestAgent);
RTTI_IMPL(WebRequestAgent, IAgent);

WebRequestAgent::~WebRequestAgent()
{}

void WebRequestAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
}

void WebRequestAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
}

bool WebRequestAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->SubscribeToType("WebRequest",
		DELEGATE(WebRequestAgent, OnWebRequest, const ThingEvent &, this), TE_ADDED);
	return true;
}

bool WebRequestAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;
	pInstance->GetBlackBoard()->UnsubscribeFromType("WebRequest", this);
	return true;
}

void WebRequestAgent::OnWebRequest(const ThingEvent & a_ThingEvent)
{
	WebRequest::SP spWebRequest = DynamicCast<WebRequest>(a_ThingEvent.GetIThing());
	if (spWebRequest)
	{
		if (!m_spActive)
		{
			ExecuteRequest(spWebRequest);
		}
		else
		{
			// request already active, just push into the queue
			m_Requests.push_back(spWebRequest);
		}
	}
}

void WebRequestAgent::ExecuteRequest(WebRequest::SP a_spWebRequest)
{
	Log::Debug("WebRequestAgent", "Executing Web Request: URL - %s%s, type - %s, body - %s", a_spWebRequest->GetURL().c_str(),
		a_spWebRequest->GetParams().c_str(), a_spWebRequest->GetType().c_str(), a_spWebRequest->GetBody().c_str());
	m_spActive = a_spWebRequest;
	m_spActive->SetState("PROCESSING");

	m_spClient = IWebClient::Request(a_spWebRequest->GetURL() + a_spWebRequest->GetParams(),
		a_spWebRequest->GetHeaders(),
		a_spWebRequest->GetType(),
		a_spWebRequest->GetBody(),
		DELEGATE(WebRequestAgent, OnResponse, IWebClient::RequestData *, this),
		DELEGATE(WebRequestAgent, OnState, IWebClient *, this));
}

void WebRequestAgent::OnResponse(IWebClient::RequestData * a_pResponse)
{
	Json::Value json;
	if (Json::Reader().parse(a_pResponse->m_Content.c_str(), json))
	{
		Goal::SP spGoal(new Goal("WebRequest"));
		spGoal->GetParams()["request_data"] = json;
		m_spActive->AddChild(spGoal);
	}
	else
	{
		Log::Error("WebRequestAgent", "Not a valid json object, cannot create new goal!");
	}

}

void WebRequestAgent::OnState(IWebClient * a_pConnector)
{
	if (a_pConnector->GetState() == IWebClient::CLOSED
		|| a_pConnector->GetState() == IWebClient::DISCONNECTED)
	{
		if (m_Requests.begin() != m_Requests.end())
		{
			WebRequest::SP spWebRequest = m_Requests.front();
			m_Requests.pop_front();
			ExecuteRequest(spWebRequest);
		}
		else
		{
			m_spActive.reset();
		}
	}
}



