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


#include "BlackBoard.h"
#include "SelfInstance.h"
#include "topics/TopicManager.h"
#include "IThing.h"

RTTI_IMPL(BlackBoard, ISerializable);
REG_SERIALIZABLE(BlackBoard);

BlackBoard::BlackBoard() : m_pTopicManager(NULL)
{
	// create our root objects
	CreateRoot("m_spPerceptionRoot", TT_PERCEPTION, m_spPerceptionRoot);
	CreateRoot("m_spCogntiveRoot", TT_COGNITIVE, m_spCogntiveRoot);
	CreateRoot("m_spModelsRoot", TT_MODEL, m_spModelsRoot);
}

void BlackBoard::Serialize(Json::Value & json)
{
	json["m_spPerceptionRoot"] = ISerializable::SerializeObject(m_spPerceptionRoot.get());
	json["m_spCogntiveRoot"] = ISerializable::SerializeObject(m_spCogntiveRoot.get());
	json["m_spModelsRoot"] = ISerializable::SerializeObject(m_spModelsRoot.get());
}

void BlackBoard::Deserialize(const Json::Value & json)
{
	DeserializeRoot("m_spPerceptionRoot", TT_PERCEPTION, json, m_spPerceptionRoot);
	DeserializeRoot("m_spCogntiveRoot", TT_COGNITIVE, json, m_spCogntiveRoot);
	DeserializeRoot("m_spModelsRoot", TT_MODEL, json, m_spModelsRoot);
}

bool BlackBoard::Start()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance != NULL)
	{
		m_pTopicManager = pInstance->GetTopicManager();

		m_pTopicManager->RegisterTopic("blackboard", "application/json",
			DELEGATE(BlackBoard, OnBlackboardSubscriber, const ITopics::SubInfo &, this));
		m_pTopicManager->RegisterTopic("blackboard-stream", "application/json",
			DELEGATE(BlackBoard, OnBlackboardStream, const ITopics::SubInfo &, this));
		// we subscribe to our own topic, this allows someone to publish to our blackboard.
		m_pTopicManager->Subscribe("blackboard",
			DELEGATE(BlackBoard, OnBlackboardInput, const ITopics::Payload &, this));

	}

	return true;
}

bool BlackBoard::Stop()
{
	if (m_pTopicManager != NULL)
	{
		m_pTopicManager->Unsubscribe("blackboard");
		m_pTopicManager->UnregisterTopic("blackboard");
		m_pTopicManager = NULL;
	}

	return true;
}

void BlackBoard::AddThing(const IThing::SP & a_spThing)
{
	if (a_spThing)
	{
		switch (a_spThing->GetCategory())
		{
		case TT_PERCEPTION:
			m_spPerceptionRoot->AddChild(a_spThing);
			break;
		case TT_COGNITIVE:
			m_spCogntiveRoot->AddChild(a_spThing);
			break;
		case TT_MODEL:
			m_spModelsRoot->AddChild(a_spThing);
			break;
		case TT_INVALID:
			Log::Error("Blackboard", "Invalid thing type %s", a_spThing->GetRTTI().GetName().c_str());
			break;
		}
	}
}

bool BlackBoard::RemoveThing(IThing * a_pThing)
{
	if (a_pThing == NULL)
		return false;
	IThing::SP spParent = a_pThing->GetParent();
	if (!spParent)
		return false;

	return spParent->RemoveChild(a_pThing);
}

void BlackBoard::OnThingEvent(const ThingEvent &thing)
{
	const IThing::SP & spThing = thing.GetIThing();
	if (spThing)
	{
		const std::string & data_type = spThing->GetDataType();
		if (data_type.size() > 0)
		{
			// use data driven type..
			SubscriberMap::iterator it = m_SubscriberMap.find(data_type);
			if (it != m_SubscriberMap.end())
			{
				SubscriberList & subs = it->second;
				for (size_t i = 0; i < subs.size(); ++i)
				{
					Subscriber & sub = subs[i];
					if ((sub.m_EventMask & (int)thing.GetThingEventType()) == 0)
						continue;
					sub.m_Callback(thing);
				}
			}
		}

		RTTI * pType = &spThing->GetRTTI();
		while (pType != NULL)
		{
			if (pType->GetName() != data_type)		// prevent double event callbacks if the data type is the same as the actual type
			{
				SubscriberMap::iterator it = m_SubscriberMap.find(pType->GetName());
				if (it != m_SubscriberMap.end())
				{
					SubscriberList & subs = it->second;
					for (size_t i = 0; i < subs.size(); ++i)
					{
						Subscriber & sub = subs[i];
						if ((sub.m_EventMask & (int)thing.GetThingEventType()) == 0)
							continue;
						sub.m_Callback(thing);
					}
				}
			}

			pType = pType->GetBaseClass();
		}

		if (m_pTopicManager != NULL && m_pTopicManager->IsSubscribed("blackboard-stream"))
		{
			Json::Value json;
			json["event"] = IThing::ThingEventTypeText(thing.GetThingEventType());

			if (thing.GetThingEventType() == TE_ADDED)
			{
				IThing::SP spParent = spThing->GetParent();
				if (spParent)
					json["parent"] = spParent->GetGUID();
				json["thing"] = ISerializable::SerializeObject(spThing.get());
			}
			else if (thing.GetThingEventType() == TE_REMOVED)
			{
				json["thing_guid"] = spThing->GetGUID();
			}
			else if (thing.GetThingEventType() == TE_STATE)
			{
				json["thing_guid"] = spThing->GetGUID();
				json["state"] = spThing->GetState();
				json["thing"] = ISerializable::SerializeObject(spThing.get());
			}
			else if (thing.GetThingEventType() == TE_IMPORTANCE)
			{
				json["thing_guid"] = spThing->GetGUID();
				json["importance"] = spThing->GetImportance();
			}
			else
				Log::Warning("BlackBoard", "Unhandled state %u", thing.GetThingEventType());

			m_pTopicManager->Publish("blackboard-stream", json.toStyledString());
		}
	}
}

void BlackBoard::SubscribeToType(const std::string & a_Type,
	Delegate<const ThingEvent &> a_callback, ThingEventType eventMask /*= TE_ALL*/)
{
	m_SubscriberMap[a_Type].push_back(Subscriber(a_callback, eventMask));
}

void BlackBoard::SubscribeToType(const RTTI & type,
	Delegate<const ThingEvent &> a_callback, ThingEventType eventMask /*= TE_ALL*/)
{
	m_SubscriberMap[type.GetName()].push_back(Subscriber(a_callback, eventMask));
}

bool BlackBoard::UnsubscribeFromType(const std::string & a_Type, void * a_pObject /*= NULL*/)
{
	if (a_pObject != NULL)
	{
		SubscriberMap::iterator iSubs = m_SubscriberMap.find(a_Type);
		if (iSubs == m_SubscriberMap.end())
			return false;		// no entry found

		SubscriberList & subs = iSubs->second;

		int erased = 0;
		for (size_t i = 0; i < subs.size();)
		{
			Subscriber & sub = subs[i];
			if (sub.m_Callback.IsObject(a_pObject))
			{
				subs.erase(subs.begin() + i);
				erased += 1;
			}
			else
				i += 1;
		}

		return erased > 0;
	}

	return m_SubscriberMap.erase(a_Type) > 0;
}

bool BlackBoard::UnsubscribeFromType(const RTTI & a_Type, void * a_pObject /*= NULL*/)
{
	return UnsubscribeFromType(a_Type.GetName(), a_pObject);
}

void BlackBoard::OnBlackboardSubscriber(const ITopics::SubInfo & a_Info)
{
	if (!a_Info.m_Subscribed)
	{
		// clean up any remote subscriptions
		if (m_RemoteSubscriberMap.find(a_Info.m_Origin) != m_RemoteSubscriberMap.end())
		{
			Log::Status("Blackboard", "Removing remote subscriber %s", a_Info.m_Origin.c_str());
			m_RemoteSubscriberMap.erase(a_Info.m_Origin);
		}
	}
}

void BlackBoard::OnBlackboardStream(const ITopics::SubInfo & a_Info)
{
	if (a_Info.m_Subscribed)
	{
		// send the entire state of the blackboard to the new subscriber..
		Json::Value json;
		json["event"] = "recv_state";
		json["state"] = ISerializable::SerializeObject(this);

		m_pTopicManager->Send(a_Info.m_Origin, "blackboard-stream", json.toStyledString());
	}
}

void BlackBoard::OnBlackboardInput(const ITopics::Payload & a_Payload)
{
	// do not act on any publish events from ourselves..
	if (a_Payload.m_RemoteOrigin[0] != 0)
	{
		Log::Debug("Blackboard", "OnBlakcboardInput(), topic: %s, data: %s",
			a_Payload.m_Topic.c_str(), a_Payload.m_Data.c_str());

		Json::Reader reader(Json::Features::strictMode());

		Json::Value json;
		if (reader.parse(a_Payload.m_Data, json)
			&& json["event"].isString())
		{
			const std::string & event = json["event"].asString();
			if (event == "add_object")
			{
				IThing::SP spParent;
				if (json.isMember("parent"))
				{
					spParent = FindThing(json["parent"].asString());
					if (!spParent)
						Log::Warning("Blackboard", "Failed to find parent by GUID.");
				}

				if ( json["thing"].isObject() )
				{
					IThing::SP spThing = IThing::SP(ISerializable::DeserializeObject<IThing>(json["thing"]));
					if (!spThing)
					{
						Log::Error("Blackboard", "Failed to deserialize thing: %s",
							json["thing"].toStyledString().c_str());
					}
					else if (spParent)
						spParent->AddChild(spThing);
					else
						AddThing(spThing);
				}
				else
					Log::Error( "Blackboard", "Parsing error, 'thing' field is not an JSON object." );
			}
			else if (event == "remove_object")
			{
				IThing::SP spThing = FindThing(json["thing_guid"].asString());
				if (spThing)
					spThing->RemoveThis();
			}
			else if (event == "get_parent")
			{
				IThing::SP spThing = FindThing(json["thing_guid"].asString());
				if (spThing)
				{
					IThing::SP spThingParent = spThing->GetParent(true);
					if (spThingParent)
					{
						std::string guid = spThingParent->GetGUID();
						Json::Value parentObj;
						parentObj["guid"] = guid;
						SelfInstance::GetInstance()->GetTopics()->Send(a_Payload.m_RemoteOrigin, "blackboard", 
							parentObj.toStyledString());
					}
				}
				else
				{
					Json::Value error;
					error["guid"] = "no_thing_found";
					SelfInstance::GetInstance()->GetTopics()->Send(a_Payload.m_RemoteOrigin, "blackboard",
						error.toStyledString());
				}
			}
			else if (event == "set_object_state")
			{
				IThing::SP spThing = FindThing(json["thing_guid"].asString());
				if (spThing)
					spThing->SetState(json["state"].asString());
			}
			else if (event == "set_object_importance")
			{
				IThing::SP spThing = FindThing(json["thing_guid"].asString());
				if (spThing)
					spThing->SetImportance(json["importance"].asFloat());
			}
			else if (event == "subscribe_to_type")
			{
				const std::string & type = json["type"].asString();
				ThingEventType event_mask = (ThingEventType)json["event_mask"].asInt();

				Log::Status("BlackBoard", "Adding Remote subscriber for type %s", type.c_str());
				m_RemoteSubscriberMap[a_Payload.m_RemoteOrigin].push_back(
					RemoteSubscriber::SP(new RemoteSubscriber(this, a_Payload.m_RemoteOrigin, type, event_mask)));
			}
			else if (event == "unsubscribe_from_type")
			{
				const std::string & type = json["type"].asString();

				RemoteSubscriberMap::iterator iSubs = m_RemoteSubscriberMap.find(a_Payload.m_RemoteOrigin);
				if (iSubs != m_RemoteSubscriberMap.end())
				{
					RemoteSubscriberList & list = iSubs->second;
					for (RemoteSubscriberList::iterator iSub = list.begin(); iSub != list.end(); ++iSub)
					{
						if ((*iSub)->m_Type == type)
						{
							Log::Status("BlackBoard", "Removing Remote subscriber for type %s", type.c_str());
							list.erase(iSub);
							break;
						}
					}

					// if this was the last one, remove it..
					if (list.end() == list.begin())
						m_RemoteSubscriberMap.erase(a_Payload.m_RemoteOrigin);
				}
			}
		}
		else
		{
			Log::Error("Blackboard", "Failed to parse blackboard event: %s", reader.getFormattedErrorMessages().c_str());
		}
	}
}


void BlackBoard::RemoteSubscriber::OnEvent(const ThingEvent & a_Event)
{
	IThing::SP spThing = a_Event.GetIThing();

	// forward event onto the origin..
	Json::Value json;
	json["event"] = IThing::ThingEventTypeText(a_Event.GetThingEventType());
	json["type"] = m_Type;

	switch (a_Event.GetThingEventType())
	{
	case TE_ADDED:
	case TE_DATA:
		{
			IThing::SP spParent = spThing->GetParent();
			if (spParent)
				json["parent"] = spParent->GetGUID();
			json["thing"] = ISerializable::SerializeObject(spThing.get());
		}
		break;
	case TE_STATE:
		json["thing_guid"] = spThing->GetGUID();
		json["state"] = spThing->GetState();
		break;
	case TE_REMOVED:
		json["thing_guid"] = spThing->GetGUID();
		break;
	case TE_IMPORTANCE:
		json["thing_guid"] = spThing->GetGUID();
		json["importance"] = spThing->GetImportance();
		break;
	case TE_GUID:
		json["thing_guid"] = spThing->GetGUID();
		json["prev_guid"] = spThing->GetPrevGUID();
		break;
	}

	m_pBlackboard->m_pTopicManager->Send(m_Origin, "blackboard", json.toStyledString());
}

void BlackBoard::CreateRoot(const std::string & a_Name, ThingCategory a_eCatgory, IThing::SP & a_spRoot)
{
	a_spRoot = IThing::SP(new IThing(a_eCatgory, -1.0f));
	a_spRoot->SetGUID(a_Name);
	a_spRoot->SetBlackBoard(this);
}

void BlackBoard::DeserializeRoot(const std::string & a_Name, ThingCategory a_eCategory,
	const Json::Value & a_Json, IThing::SP & a_spRoot)
{
	a_spRoot = IThing::SP(ISerializable::DeserializeObject<IThing>(a_Json[a_Name]));
	if (!a_spRoot)
	{
		Log::Warning("BlackBoard", "Failed to deserialize root %s", a_Name.c_str());
		CreateRoot(a_Name, a_eCategory, a_spRoot);
	}
	else
	{
		if (a_spRoot->GetCategory() != a_eCategory)		// TODO: remove later, this is only here because I changed the variable name
			a_spRoot->SetCategory(TT_PERCEPTION);
		a_spRoot->SetBlackBoard(this);
	}
}


