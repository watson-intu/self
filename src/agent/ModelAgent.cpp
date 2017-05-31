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


#include "ModelAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"

REG_SERIALIZABLE(ModelAgent);
RTTI_IMPL(ModelAgent, IAgent);

ModelAgent::ModelAgent()
{}

void ModelAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
}

void ModelAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
}

bool ModelAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert(pBlackboard != NULL);
	IGraph::SP spGraph = pInstance->GetKnowledgeGraph();
	assert(spGraph.get() != NULL);
	ITopics * pTopics = pInstance->GetTopics();
	assert(pTopics != NULL);

	pBlackboard->SubscribeToType("ModelQuery",
		DELEGATE(ModelAgent, OnQuery, const ThingEvent &, this));
	pTopics->RegisterTopic("models", "application/json",
		DELEGATE(ModelAgent, OnModelSubscriber, const ITopics::SubInfo &, shared_from_this()));
	pTopics->Subscribe("models",
		DELEGATE(ModelAgent, OnModelPayload, const ITopics::Payload &, shared_from_this()));

	return true;
}

bool ModelAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert(pBlackboard != NULL);
	IGraph::SP spGraph = pInstance->GetKnowledgeGraph();
	assert(spGraph.get() != NULL);
	ITopics * pTopics = pInstance->GetTopics();
	assert(pTopics != NULL);

	pBlackboard->UnsubscribeFromType("ModelQuery", this);
	pTopics->Unsubscribe("models", this);
	pTopics->UnregisterTopic("models");

	return true;
}

void ModelAgent::OnQuery(const ThingEvent & a_ThingEvent)
{
	if (a_ThingEvent.GetThingEventType() == TE_ADDED)
		new ModelQuery(this, a_ThingEvent.GetIThing());
}

void ModelAgent::OnModelSubscriber(const ITopics::SubInfo & a_Info)
{
	if (!a_Info.m_Subscribed)
	{
		SubscriberMap::iterator iSubscriber = m_SubscriberMap.find(a_Info.m_Origin);
		if (iSubscriber != m_SubscriberMap.end())
		{
			// unsubscribe from all the objects we subscribed, so we don't crash when the GraphSubscriber object is destroyed.
			ModelSubscriber * pSubscriber = &iSubscriber->second;
			for (size_t i = 0; i < pSubscriber->m_Objects.size(); ++i)
			{
				IThing::SP spThing = pSubscriber->m_Objects[i].lock();
				if (spThing)
					spThing->Unsubscribe(this);
			}

			m_SubscriberMap.erase(iSubscriber);
		}
	}
}

void ModelAgent::OnModelPayload(const ITopics::Payload & a_Payload)
{
	// firstly, check that this payload arrived from a remote source..
	if (a_Payload.m_RemoteOrigin[0] != 0)
	{
		SubscriberMap::iterator iSubscriber = m_SubscriberMap.find(a_Payload.m_RemoteOrigin);
		if (iSubscriber == m_SubscriberMap.end())
		{
			ModelSubscriber & newSub = m_SubscriberMap[a_Payload.m_RemoteOrigin];
			newSub.m_TopicId = a_Payload.m_Topic;
			newSub.m_Origin = a_Payload.m_RemoteOrigin;

			Log::Status("ModelAgent", "New remote subscriber '%s'.", a_Payload.m_RemoteOrigin.c_str());
			iSubscriber = m_SubscriberMap.find(a_Payload.m_RemoteOrigin);
			assert(iSubscriber != m_SubscriberMap.end());
		}
		ModelSubscriber * pSubscriber = &iSubscriber->second;
		assert(pSubscriber != NULL);

		Json::Value json;
		Json::Reader reader(Json::Features::strictMode());
		if (reader.parse(a_Payload.m_Data, json) && json["data"].isString())
		{
			Json::Value data;
			if (reader.parse(json["data"].asString(), data))
			{
				IThing::SP spGraphQuery(new IThing(TT_MODEL, "ModelQuery", data));
				spGraphQuery->Subscribe(DELEGATE(ModelSubscriber, OnThingEvent, const ThingEvent &, pSubscriber));
				SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spGraphQuery);

				// add into a list so we can unsubscribe later..
				pSubscriber->m_Objects.push_back(spGraphQuery);
			}
			else
				Log::Error("ModelAgent", "Failed to parse JSON data: %s", reader.getFormattedErrorMessages().c_str());
		}
		else
			Log::Error("ModelAgent", "Failed to parse JSON payload: %s", reader.getFormattedErrorMessages().c_str());
	}
}

//-----------------------------------------------------------

ModelAgent::ModelQuery::ModelQuery(ModelAgent * a_pAgent, const IThing::SP & a_spQuery) : m_pAgent(a_pAgent), m_spQuery(a_spQuery)
{
	bool bError = true;
	bool bCompleted = false;

	Json::Value & data = m_spQuery->GetData();
	if (data["event"].isString())
	{
		IGraph::SP spGraph = SelfInstance::GetInstance()->GetKnowledgeGraph();
		assert(spGraph.get() != NULL);

		std::string model("world");
		if (data.isMember("model"))
			model = data["model"].asString();

		spGraph->SetModel(model);

		std::string evt = data["event"].asString();
		if (evt == "traverse")
		{
			ITraverser::SP spTraverser = spGraph->LoadTraverser((*a_spQuery)["traverser"]);
			if (spTraverser)
				bError = !spTraverser->Start(DELEGATE(ModelQuery, OnTraverse, ITraverser::SP, this));
		}
		else if (evt == "gremlin")
		{
			Graph * pGraphService = SelfInstance::GetInstance()->FindService<Graph>();
			if (pGraphService != NULL)
			{
				std::string graphId = data["graph_id"].asString();
				std::string query = data["query"].asString();
				Json::Value bindings = data["bindings"];

				bError = pGraphService->Query(graphId, query, bindings,
					DELEGATE(ModelQuery, OnGremlinQuery, const Json::Value &, this)) == false;
			}
		}
		else if (evt == "create_vertex")
		{
			std::string label(data["label"].asString());
			Json::Value props(data["properties"]);

			IVertex::SP spVertex = spGraph->CreateVertex(label, props,
				DELEGATE(ModelQuery, OnVertexEvent, const IVertex::VertexEvent &, this));
			if (spVertex)
				bError = false;
		}
		else if (evt == "update_vertex")
		{
			std::string id(data["vertex_id"].asString());
			Json::Value props(data["properties"]);

			IVertex::SP spVertex = spGraph->FindVertex(id);
			if (spVertex)
			{
				spVertex->SetProperties(props);
				spVertex->Save();
				bCompleted = true;
			}
		}
		else if (evt == "drop_vertex")
		{
			std::string id(data["vertex_id"].asString());

			IVertex::SP spVertex = spGraph->FindVertex(id);
			if (spVertex)
			{
				spVertex->Drop();
				bCompleted = true;
			}
		}
		else if (evt == "create_edge")
		{
			std::string label(data["label"].asString());
			std::string sourceId(data["source_id"].asString());
			std::string destId(data["destination_id"].asString());
			Json::Value props(data["properties"]);

			IVertex::SP spSource = spGraph->FindVertex(sourceId);
			if (spSource)
			{
				IVertex::SP spDest = spGraph->FindVertex(destId);
				if (spDest)
				{
					IEdge::SP spEdge = spGraph->CreateEdge(label, spSource, spDest, props,
						DELEGATE(ModelQuery, OnEdgeEvent, const IEdge::EdgeEvent &, this));
					if (spEdge)
						bError = false;
				}
				else
					Log::Error("ModelAgent", "Failed to find destination vertex %s", destId.c_str());
			}
			else
				Log::Error("ModelAgent", "Failed to find source vertex %s", sourceId.c_str());
		}
		else if (evt == "update_edge")
		{
			std::string id(data["edge_id"].asString());
			Json::Value props(data["properties"]);

			IEdge::SP spEdge = spGraph->FindEdge(id);
			if (spEdge)
			{
				spEdge->SetProperties(props);
				spEdge->Save();
				bCompleted = true;
			}
		}
		else if (evt == "drop_edge")
		{
			std::string id(data["edge_id"].asString());

			IEdge::SP spEdge = spGraph->FindEdge(id);
			if (spEdge)
			{
				spEdge->Drop();
				bCompleted = true;
			}
		}
		else
			Log::Error("ModelAgent", "Unknown event type '%s'", evt.c_str());
	}

	if (bCompleted)
	{
		m_spQuery->SetState("COMPLETED");
		delete this;
	}
	else if (bError)
	{
		Log::Error("ModelAgent", "ModelQuery failed.");
		m_spQuery->SetState("FAILED");
		delete this;
	}
}

void ModelAgent::ModelQuery::OnTraverse(ITraverser::SP a_spTraverser)
{
	// store results into the IThing object, then notify observers with a data and state change events..
	for (size_t i = 0; i < a_spTraverser->Size(); ++i)
		a_spTraverser->GetResult(i)->Serialize(m_spQuery->GetData()["results"][i]);
	m_spQuery->OnDataChanged();
	m_spQuery->SetState("COMPLETED");
	delete this;
}

void ModelAgent::ModelQuery::OnVertexEvent(const IVertex::VertexEvent & a_Event)
{
	if (a_Event.m_Type == IVertex::E_COMMITTED || a_Event.m_Type == IVertex::E_DROPPING)
	{
		static const char * VERTEX_EVENTS[] =
		{
			"COMMITTED",	// E_COMMITED
			"MODIFIED",		// E_MODIFIED
			"DROPPING"		// E_DROPPING
		};

		a_Event.m_spVertex->GetNotificationList().Remove(this);
		m_spQuery->GetData()["vertex"] = ISerializable::SerializeObject(a_Event.m_spVertex.get());
		m_spQuery->GetData()["vertex_event"] = VERTEX_EVENTS[a_Event.m_Type];
		m_spQuery->OnDataChanged();
		m_spQuery->SetState("COMPLETED");
		delete this;
	}
}

void ModelAgent::ModelQuery::OnEdgeEvent(const IEdge::EdgeEvent & a_Event)
{
	if (a_Event.m_Type == IEdge::E_COMMITTED || a_Event.m_Type == IEdge::E_DROPPING)
	{
		static const char * EDGE_EVENTS[] =
		{
			"COMMITTED",	// E_COMMITED
			"MODIFIED",		// E_MODIFIED
			"DROPPING"		// E_DROPPING
		};
		a_Event.m_spEdge->GetNotificationList().Remove(this);
		m_spQuery->GetData()["edge"] = ISerializable::SerializeObject(a_Event.m_spEdge.get());
		m_spQuery->GetData()["edge_event"] = EDGE_EVENTS[a_Event.m_Type];
		m_spQuery->OnDataChanged();
		m_spQuery->SetState("COMPLETED");
		delete this;
	}
}

void ModelAgent::ModelQuery::OnGremlinQuery(const Json::Value & a_Result)
{
	if (!a_Result.isNull())
	{
		m_spQuery->GetData()["result"] = a_Result;
		m_spQuery->OnDataChanged();
		m_spQuery->SetState("COMPLETED");
	}
	else
		m_spQuery->SetState("FAILED");
	delete this;
}

void ModelAgent::ModelSubscriber::OnThingEvent(const ThingEvent & a_Event)
{
	IThing::SP spThing = a_Event.GetIThing();
	assert(spThing.get() != NULL);

	Json::Value json;
	json["event"] = IThing::ThingEventTypeText(a_Event.GetThingEventType());

	switch (a_Event.GetThingEventType())
	{
	case TE_ADDED:
	case TE_DATA:
		json["thing"] = ISerializable::SerializeObject(spThing.get());
		break;
	case TE_STATE:
		json["thing_guid"] = spThing->GetGUID();
		json["state"] = spThing->GetState();
		break;
	case TE_REMOVED:
		json["thing_guid"] = spThing->GetGUID();
		break;
	}

	SelfInstance::GetInstance()->GetTopics()->Send(m_Origin, m_TopicId, json.toStyledString());
}
