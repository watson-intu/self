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


#ifndef SELF_GRAPH_H
#define SELF_GRAPH_H

#include <map>
#include <set>

#include "SelfEdge.h"
#include "SelfVertex.h"

#include "utils/ISerializable.h"
#include "models/IGraph.h"
#include "topics/ITopics.h"
#include "utils/UniqueID.h"
#include "utils/TimerPool.h"
#include "utils/IDataStore.h"
#include "services/Graph/Graph.h"
#include "SelfLib.h"

//! Concrete implementation of the IGraph interface that uses a local DB for caching vertexes and edges locally, but
//! will use the Graph service to access a larger cloud based graph.
class SELF_API SelfGraph : public IGraph
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<SelfGraph>			SP;
	typedef boost::weak_ptr<SelfGraph>				WP;
	typedef unsigned int							EventId;

	//! Construction
	SelfGraph();
	SelfGraph(const std::string & a_GraphId);
	~SelfGraph();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IGraph interface
	virtual bool		Load( OnGraphLoaded a_Callback );
	virtual bool		Connect(OnGraphConnected a_Callback,
		const std::string & a_ServiceId = "GraphV1");
	virtual bool		Import(const Json::Value & a_Import);
	virtual bool		Export(Json::Value & a_Export);
	virtual void		Clear();
	virtual bool		Close();
	virtual bool		Drop();

	virtual void		SetModel(const std::string & a_Group);
	virtual ITraverser::SP CreateTraverser(const Condition & a_spCond = ITraverser::NULL_CONDITION);
	virtual ITraverser::SP LoadTraverser(const Json::Value & a_Data);
	virtual IVertex::SP CreateVertex(const std::string & a_Label,
		const PropertyMap & a_Properties,
		VertexEvent a_Callback = VertexEvent());
	virtual IVertex::SP FindVertex(const VertexId & a_Id);
	virtual IEdge::SP	CreateEdge(const std::string & a_Label,
		const IVertex::SP & a_Src,
		const IVertex::SP & a_Dst,
		const PropertyMap & a_Properties,
		EdgeEvent a_Callback = EdgeEvent());
	virtual IEdge::SP	FindEdge(const VertexId & a_Id);

	virtual std::string ToString();

protected:
	//! Types
	typedef std::map<VertexId, SelfVertex::SP>			VertexMap;
	typedef std::map<std::string, IVertex::SP>			GroupMap;
	typedef std::map<EdgeId, SelfEdge::SP>				EdgeMap;

	//! Data
	bool				m_bLoading;
	bool				m_bLoaded;
	bool				m_bCreated;
	bool				m_bError;
	Graph *				m_pGraph;				// our graph service if available
	IDataStore *		m_pStorage;				// local data storage for graph data
	OnGraphLoaded		m_OnGraphLoaded;
	OnGraphConnected	m_OnGraphConnected;
	volatile size_t		m_nPendingOps;

	GroupMap			m_Models;				// map of all available models
	IVertex::SP			m_ActiveModel;			// currently active model

	VertexMap			m_VertexMap;			// local vertex data, used only if remote graph is not available
	EdgeMap				m_EdgeMap;

	void				OnGraphCreated(const Json::Value & a_Result);
	void				OnGraphDeleted(const Json::Value & a_Result);
	void				OnLoadVertex(Json::Value * a_Json);
	void				OnLoadEdge(Json::Value * a_Json);
	void				OnLoadDone();

	void				DiscoverModels();
	void				OnDiscoverModels(ITraverser::SP a_spTraverser);

	friend class ISelfTraverser;
	friend class SelfVertex;
	friend class SelfEdge;
	friend class FilterTraverser;
	friend class OutTraverser;
	friend class InTraverser;
};

#endif // SELF_LOCAL_GRAPH_H

