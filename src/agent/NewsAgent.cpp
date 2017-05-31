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


#include "NewsAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Goal.h"
#include "services/INews.h"
#include "SelfInstance.h"

REG_SERIALIZABLE(NewsAgent);
RTTI_IMPL(NewsAgent, IAgent);

void NewsAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
	json["m_NumberOfArticles"] = m_NumberOfArticles;
}

void NewsAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
	if (json.isMember("m_NumberOfArticles"))
		m_NumberOfArticles = json["m_NumberOfArticles"].asInt();
}

bool NewsAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->SubscribeToType("NewsIntent",
		DELEGATE(NewsAgent, OnNewsIntent, const ThingEvent &, this), TE_ADDED);
	return true;
}

bool NewsAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;
	pInstance->GetBlackBoard()->UnsubscribeFromType("NewsIntent", this);
	return true;
}

void NewsAgent::OnNewsIntent(const ThingEvent & a_ThingEvent)
{
	NewsIntent::SP spNewsIntent = DynamicCast<NewsIntent>(a_ThingEvent.GetIThing());
	if (spNewsIntent)
	{
		bool bFailed = true;

		INews * pService = Config::Instance()->FindService<INews>();
		if (pService != NULL)
		{
			m_spActive = spNewsIntent;

			if (!spNewsIntent->GetCompany().empty())
			{
				time_t endTime = (time_t)Time().GetEpochTime();
				time_t startTime = endTime - (687600 * 2);
				Log::Debug("NewsAgent", "Start Time: %f, End Time: %f", startTime, endTime);
				m_spActive->SetState("PROCESSING");

				boost::shared_ptr<NewsAgent> spThis(boost::static_pointer_cast<NewsAgent>(shared_from_this()));
				pService->GetNews(spNewsIntent->GetCompany(), startTime, endTime, m_NumberOfArticles,
					DELEGATE(NewsAgent, OnNewsData, const Json::Value &, spThis));
				bFailed = false;
			}
		}

		if (bFailed)
			spNewsIntent->SetState("FAILED");
	}
}

void NewsAgent::OnNewsData(const Json::Value & a_Callback)
{
	Log::Debug("NewsAgent", "OnNewsData(): %s", a_Callback.toStyledString().c_str());

	Goal::SP spGoal(new Goal("News"));
	spGoal->GetParams()["news_data"] = a_Callback;

	m_spActive->AddChild(spGoal);
	m_spActive->SetState("COMPLETED");
}
