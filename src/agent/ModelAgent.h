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


#ifndef SELF_OTHERS_AGENT_H
#define SELF_OTHERS_AGENT_H

#include "IAgent.h"
#include "blackboard/IThing.h"
#include "services/Graph/Graph.h"
#include "SelfLib.h"

//! This agent handles synchronization of the blackboard with the graph using a model query language.
//! Additionally, it will handle specific requests from other agents to query and modify the graph.
class SELF_API ModelAgent : public IAgent
{
public:
	RTTI_DECL();

	//! Construction
	ModelAgent();

	//! ISerializable interface
	void Serialize( Json::Value & json );
	void Deserialize( const Json::Value & json );

	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

	boost::shared_ptr<ModelAgent> shared_from_this()
	{
		return boost::static_pointer_cast<ModelAgent>( IAgent::shared_from_this() );
	}
private:
	//! Types
	struct ModelQuery 
	{
		ModelQuery( ModelAgent * a_pAgent, const IThing::SP & a_spQuery );
		void OnTraverse( ITraverser::SP a_spTraverser );
		void OnVertexEvent( const IVertex::VertexEvent & a_Event );
		void OnEdgeEvent( const IEdge::EdgeEvent & a_Event );
		void OnGremlinQuery( const Json::Value & a_Result );

		ModelAgent *	m_pAgent;
		IThing::SP		m_spQuery;
	};
	typedef std::vector<IThing::WP>						ObjectList;

	struct ModelSubscriber
	{
		ObjectList		m_Objects;
		std::string		m_TopicId;
		std::string		m_Origin;

		void OnThingEvent( const ThingEvent & a_Event );
	};
	typedef std::map<std::string,ModelSubscriber>		SubscriberMap;

	//! Data
	SubscriberMap		m_SubscriberMap;

	//! Callbacks
	void OnModelSubscriber( const ITopics::SubInfo & a_Info );
	void OnModelPayload( const ITopics::Payload & a_Payload );
	void OnQuery(const ThingEvent & a_ThingEvent);
};

#endif //SELF_OTHERS_AGENT_H
