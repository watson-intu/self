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


#ifndef SELF_IGRAPH_H
#define SELF_IGRAPH_H

#include <boost/enable_shared_from_this.hpp>
#include <list>
#include <algorithm>

#include "topics/ITopics.h"
#include "utils/Delegate.h"
#include "utils/ISerializable.h"
#include "utils/IConditional.h"
#include "utils/WatsonException.h"
#include "utils/Factory.h"

#include "IVertex.h"
#include "IEdge.h"
#include "ITraverser.h"

#include "SelfLib.h"

//! This class wraps the graph implementation that is used by self.
class SELF_API IGraph : public ISerializable, 
	public boost::enable_shared_from_this<IGraph>
{
public:
	RTTI_DECL();

	//! Create a IGraph
	static Factory<IGraph> & GetGraphFactory();
	static IGraph * Create( const std::string & a_GraphType, const std::string & a_GraphId );

	//! Types
	typedef boost::shared_ptr<IGraph>					SP;
	typedef boost::weak_ptr<IGraph>						WP;

	typedef Delegate<IGraph::SP>						OnGraphConnected;
	typedef Delegate<IGraph::SP>						OnGraphLoaded;
	typedef Delegate<const IVertex::VertexEvent &>		VertexEvent;
	typedef Delegate<const IEdge::EdgeEvent &>			EdgeEvent;

	typedef IVertex::VertexId							VertexId;
	typedef IEdge::EdgeId								EdgeId;
	typedef IConditional								Condition;
	typedef IVertex::PropertyMap						PropertyMap;

	typedef std::vector<IVertex::SP>					VertexList;
	typedef std::vector<IEdge::SP>						EdgeList;

	//! Construction
	IGraph()
	{}
	IGraph( const std::string & a_GraphId ) : m_GraphId( a_GraphId )
	{}
	virtual ~IGraph()
	{}

	const std::string & GetGraphId() const { return m_GraphId; }
	void SetGraphId( const std::string & a_GraphId ) { m_GraphId = a_GraphId; }

	//! Load the local storage for this graph, it is ready to be queried but all queries 
	//! will be against the local graph only until Connect() is invoked.
	virtual bool		Load( OnGraphLoaded a_Callback ) = 0;
	//! Connect this graph to remote storage via the provided service.
	virtual bool		Connect( OnGraphConnected a_Callback,
							const std::string & a_ServiceId = "GraphV1" ) = 0;
	//! Import a graph from the provided JSON
	virtual bool		Import( const Json::Value & a_Import ) = 0;
	//! Export this graph into a flat file.
	virtual bool		Export( Json::Value & a_Export ) = 0;
	//! Wipe all data from this graph.
	virtual void		Clear() = 0;
	//! Disconnect this graph from remote & local storage.
	virtual bool		Close() = 0;
	//! Close and destroy this graph from storage.
	virtual bool		Drop() = 0;

	//! Set the current model, any created vertex will be associated with this model within the graph.
	virtual void		SetModel( const std::string & a_Model ) = 0;
	//! Create a new traverser object for searching this graph.
	virtual ITraverser::SP CreateTraverser(const Condition & a_spCond = ITraverser::NULL_CONDITION) = 0;
	//! Load a export traverser
	virtual ITraverser::SP LoadTraverser( const Json::Value & a_Data ) = 0;
	//! Find/Create a new vertex in this graph, returns the IVert object.
	virtual IVertex::SP CreateVertex( const std::string & a_Label, 
							const PropertyMap & a_Properties,
							VertexEvent a_Callback = VertexEvent() ) = 0;
	//! Find a vertex in the local graph cache
	virtual IVertex::SP FindVertex( const VertexId & a_Id ) = 0;
	//! Create a new edge connecting the two specified vertexes
	virtual IEdge::SP	CreateEdge( const std::string & a_Label, 
							const IVertex::SP & a_Src, 
							const IVertex::SP & a_Dst, 
							const PropertyMap & a_Properties,
							EdgeEvent a_Callback = EdgeEvent() ) = 0;
	//! Find an edge in the local graph cache
	virtual IEdge::SP	FindEdge( const VertexId & a_Id ) = 0;

	//! Dump graph into string for debugging purposes.
	virtual std::string ToString() = 0;

protected:
	std::string			m_GraphId;

	friend class ITraverser;
};

//----------------------------

class SELF_API LabelCondition : public EqualityCondition
{
public:
	RTTI_DECL();

	LabelCondition()
	{}
	LabelCondition(const std::string & a_Label) : EqualityCondition("_label", Logic::EQ, a_Label)
	{}
};

#endif // SELF_IGRAPH_H
