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


#include "SelfTraverser.h"
#include "SelfGraph.h"

RTTI_IMPL(ISelfTraverser, ITraverser);

RTTI_IMPL(FilterTraverser, ISelfTraverser);
REG_SERIALIZABLE(FilterTraverser);
RTTI_IMPL(OutTraverser, ISelfTraverser);
REG_SERIALIZABLE(OutTraverser);
RTTI_IMPL(InTraverser, ISelfTraverser);
REG_SERIALIZABLE(InTraverser);

//----------------------------------------------

bool ISelfTraverser::Export( Json::Value & a_Export )
{
	a_Export = ISerializable::SerializeObject( this );
	return true;
}

ITraverser::SP ISelfTraverser::Filter(const Condition & a_spCond)
{
	return ITraverser::SP(new FilterTraverser(a_spCond, shared_from_this()));
}

ITraverser::SP ISelfTraverser::Out(const Condition & a_spCond /*= NULL_CONDITION*/)
{
	return ITraverser::SP(new OutTraverser(a_spCond, shared_from_this()));
}

ITraverser::SP ISelfTraverser::In(const Condition & a_spCond /*= NULL_CONDITION*/)
{
	return ITraverser::SP(new InTraverser(a_spCond, shared_from_this()));
}

bool ISelfTraverser::Start(TraverseCallback a_Callback)
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	if ( pGraph == NULL )
	{
		Log::Error("ISelfTraverser", "Traverser not associated with a graph object.");
		return false;
	}

	m_Callback = a_Callback;
	m_Results.clear();

	// If we have a remote graph, send our traverse request to that graph..
	if ( pGraph->m_pGraph != NULL && !m_bRemoteDone )
		m_bRemoteTraverse = SendGremlinQuery();

	if ( m_spNext )
	{
		if (! m_spNext->Start( DELEGATE( ISelfTraverser, OnNextDone, ITraverser::SP, shared_from_this() ) ) )
		{
			Log::Error( "ISelfTraverser", "Failed to execute next traverser." );
			return false;
		}
	}
	else
	{
		ExecuteTraverse();
	}

	return true;
}

void ISelfTraverser::ExecuteTraverse()
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	assert( pGraph != NULL );

	// if this is a top level filter, then pull all vertex objects from the local graph..
	if (! m_spNext )
	{
		SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
		assert( pGraph );

		m_Results.clear();
		for( SelfGraph::VertexMap::const_iterator iVertex = pGraph->m_VertexMap.begin(); 
			iVertex != pGraph->m_VertexMap.end(); ++iVertex )
		{
			m_Results.push_back( iVertex->second );
		}

	}

	// traverse the local graph..
	OnLocalTraverse();

	// if we got any results or we didn't send a remote traversal, then invoke the callback
	if ( m_Results.size() > 0 || !m_bRemoteTraverse )
	{
		m_bLocalTraverse = true;
		m_Callback( shared_from_this() );
		m_Callback.Reset();
	}
}

bool ISelfTraverser::SendGremlinQuery()
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	assert( pGraph );

	if ( pGraph->m_pGraph == NULL )
		return false;

	m_Query = "graph.traversal().V()";

	if ( pGraph->m_ActiveModel )
	{
		m_Bindings["modelId"] = (*pGraph->m_ActiveModel)["modelId"];
		m_Query += ".has('modelId',modelId).outE().has('_label','aspect').inV()";
	}

	std::list<ISelfTraverser::SP> traversers;

	// we need to resolve them in reverse order, so move them into a list pushing to the front, then enumerate from the head..
	ISelfTraverser::SP spTraverse = shared_from_this();
	while( spTraverse )
	{
		traversers.push_front( spTraverse );
		spTraverse = DynamicCast<ISelfTraverser>( spTraverse->m_spNext );
	}
	for( std::list<ISelfTraverser::SP>::iterator iTraverse = traversers.begin(); 
		iTraverse != traversers.end(); ++iTraverse )
	{
		(*iTraverse)->BuildGremlinQuery( m_Query, m_Bindings );
		(*iTraverse)->m_bRemoteDone = true;		// set the flag as we traverse, we only want to do one gremlin query for a chain..
	}

	// this gives us all the edges and vertexes visited during this query so we can cache them locally..
	m_Query += ".path()";		

	pGraph->m_pGraph->Query( pGraph->m_GraphId, m_Query, m_Bindings, 
		DELEGATE( ISelfTraverser, OnGremlinQuery, const Json::Value &, this ) );

	m_spThis = shared_from_this();
	return true;
}

void ISelfTraverser::OnNextDone( ITraverser::SP a_Results)
{
	// our next traverser is done, pull it's results into ours then run the traverse
	ISelfTraverser * pResult = DynamicCast<ISelfTraverser>( a_Results.get() );
	m_Results.swap( pResult->m_Results );	

	ExecuteTraverse();
}

void ISelfTraverser::OnGremlinQuery( const Json::Value & a_Result )
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	assert( pGraph != NULL );

	Log::Debug( "ISelfTraverser", "OnGremlinQuery: %s\nResult:\n%s", 
		m_Query.c_str(), a_Result.toStyledString().c_str() );

	std::set<std::string> foundVertexes;
	if (! a_Result.isNull() )
	{
		const Json::Value & data = a_Result["result"]["data"];
		for(size_t i=0;i<data.size();++i)
		{
			// do vertexes first
			const Json::Value & objects = data[i]["objects"];
			for(size_t k=0;k<objects.size();++k)
			{
				const Json::Value & object = objects[k];
				std::string type( object["type"].asString() );
				if ( type == "vertex" ) 
				{
					std::string id( object["id"].asString() );
					std::string label( object["label"].asString() );

					SelfVertex::SP spVertex;
					SelfGraph::VertexMap::iterator iVertex = pGraph->m_VertexMap.find( id );
					if ( iVertex == pGraph->m_VertexMap.end() )
					{
						spVertex = SelfVertex::SP( new SelfVertex() );
						spVertex->SetGraph( pGraph );
						spVertex->SetId( id );
						spVertex->SetLabel( label );
					}
					else
						spVertex = iVertex->second;

					Json::Value newProps;
					const Json::Value & props = object["properties"];
					for( Json::ValueConstIterator iProp = props.begin(); iProp != props.end(); ++iProp )
						newProps[ iProp.name() ] = (*iProp)[0]["value"];

					spVertex->SetProperties( newProps );
					spVertex->SaveLocal();

					foundVertexes.insert( id );
				}
			}

			// now do the edges, we do this after all the vertexes so we can actually find them in the map..
			for(size_t k=0;k<objects.size();++k)
			{
				const Json::Value & object = objects[k];
				std::string type( object["type"].asString() );
				if ( type == "edge" )
				{
					std::string id( object["id"].asString() );
					std::string label( object["label"].asString() );
					std::string inV( object["inV"].asString() );
					std::string outV( object["outV"].asString() );

					SelfGraph::VertexMap::iterator iSource = pGraph->m_VertexMap.find( outV );
					if ( iSource == pGraph->m_VertexMap.end() )
					{
						Log::Warning( "ISelfTraverser", "Failed to find outV %s", outV.c_str() );
						continue;
					}
					SelfGraph::VertexMap::iterator iDest = pGraph->m_VertexMap.find( inV );
					if ( iDest == pGraph->m_VertexMap.end() )
					{
						Log::Warning( "ISelfTraverser", "Failed to find inV %s", inV.c_str() );
						continue;
					}

					SelfEdge::SP spEdge;
					SelfGraph::EdgeMap::iterator iEdge = pGraph->m_EdgeMap.find( id );
					if ( iEdge == pGraph->m_EdgeMap.end() )
					{
						spEdge = SelfEdge::SP( new SelfEdge() );
						spEdge->SetGraph( pGraph );
						spEdge->SetId( id );
						spEdge->SetLabel( label );
						spEdge->SetSource( iSource->second );
						spEdge->SetDestination( iDest->second );
					}
					else
						spEdge = iEdge->second;

					Json::Value newProps;
					const Json::Value & props = object["properties"];
					for( Json::ValueConstIterator iProp = props.begin(); iProp != props.end(); ++iProp )
						newProps[ iProp.name() ] = (*iProp);
					spEdge->SetProperties( newProps );
					spEdge->SaveLocal();
				}
			}
		}

		// check the local results, remove any vertexes we didn't find in the remote traverse
		if ( m_bLocalTraverse )
		{
			for(size_t k=0;k<m_Results.size();++k)
			{
				if ( foundVertexes.find( m_Results[k]->GetId() ) == foundVertexes.end() )
				{
					Log::Status( "SelfTraverser", "Removing local only vertex: %s", m_Results[k]->ToJson().toStyledString().c_str() );
					m_Results[k]->Drop();
				}
			}
		}

	}
	else
	{
		Log::Error( "ISelfTraverser", "Invalid query results for query: %s", m_Query.c_str() );
	}

	if (m_Callback.IsValid() && m_bRemoteTraverse )
	{
		// remove traverse is done, the results should now be in our local 
		// graph, so run our search a 2nd time..
		m_bRemoteDone = true;
		m_bRemoteTraverse = false;
		ExecuteTraverse();
	}
	
	m_spThis.reset();
}

const char * ISelfTraverser::GetEqualityOp(Logic::EqualityOp a_Op)
{
	static const char * OPS[] =
	{
		"eq(%s)",		// ==
		"neq(%s)",		// !=
		"gt(%s)",		// >
		"gte(%s)",		// >=
		"lt(%s)",		// <
		"lte(%s)",		// <=
		"eq(%s)"		// LIKE
	};
	int index = (int)a_Op;
	if (index < 0 || index >= sizeof(OPS) / sizeof(OPS[0]))
		index = 0;
	return OPS[index];
}

const char * ISelfTraverser::GetLogicalOp(Logic::LogicalOp a_LogOp)
{
	static const char * OPS[] =
	{
		"and(%s)",			// AND
		"or(%s)",			// OR
		"not(%s)"			// NOT
	};
	int index = (int)a_LogOp;
	if (index < 0 || index >= sizeof(OPS) / sizeof(OPS[0]))
		index = 0;

	return OPS[index];
}

// graph.traversal().V().has('sex','male').and( has('age',gt(40)), has('age',lt(55)) );
// graph.traversal().V().has('name','Richard');
// graph.traversal().V().has('name','Richard').out("team");

std::string ISelfTraverser::GremlinCondition( IConditional::SP a_spCondition, Json::Value & a_Bindings )
{
	std::string cond;
	if ( a_spCondition->GetRTTI() == LogicalCondition::GetStaticRTTI())
	{
		LogicalCondition * pLogicalCond = (LogicalCondition *)a_spCondition.get();

		std::string subcond;
		for(size_t i=0;i<pLogicalCond->m_Conditions.size();++i)
		{
			if ( subcond.size() > 0 )
				subcond += ",";
			subcond += GremlinCondition( pLogicalCond->m_Conditions[i], a_Bindings );
		}

		cond = StringUtil::Format( GetLogicalOp( pLogicalCond->m_LogicOp ), subcond.c_str() );
	}
	else if ( a_spCondition->GetRTTI().IsType( &EqualityCondition::GetStaticRTTI()) )
	{
		EqualityCondition * pEqCond = (EqualityCondition *)a_spCondition.get();
		if (!pEqCond->m_Value.isObject() && !pEqCond->m_Value.isArray())
		{
			std::string binding( StringUtil::Format("bind%d", a_Bindings.size() ) );

			a_Bindings[binding] = pEqCond->m_Value;
			cond = "has('" + pEqCond->m_Path + "'," + StringUtil::Format( GetEqualityOp( pEqCond->m_EqualOp ), binding.c_str() ) + ")";
		}
	}

	return cond;
}

//----------------------------------------------

void FilterTraverser::BuildGremlinQuery( std::string & a_Query, Json::Value & a_Bindings )
{
	if ( m_spCondition )
	{
		std::string cond = GremlinCondition( m_spCondition, a_Bindings );
		if ( cond.size() > 0 )
			a_Query += "." + cond;
	}
}

void FilterTraverser::OnLocalTraverse()
{
	VertextList results;
	for(size_t i=0;i<m_Results.size();++i)
	{
		const IVertex::SP & spVertex = m_Results[i];
		if (! spVertex)
			continue;
		if ( m_spCondition && !m_spCondition->Test( spVertex->GetProperties() ) )
			continue;
		results.push_back( spVertex );
	}
	m_Results.swap( results );
}

//----------------------------------------------

void OutTraverser::BuildGremlinQuery( std::string & a_Query, Json::Value & a_Bindings )
{
	a_Query += ".outE()";
	if ( m_spCondition )
	{
		std::string cond = GremlinCondition( m_spCondition, a_Bindings );
		if ( cond.size() > 0 )
			a_Query += "." + cond;
	}
	a_Query += ".inV()";
}

//! This traverser builds a graph where the out-edge has the specified conditions
void OutTraverser::OnLocalTraverse()
{
	VertextList results;
	for (size_t i = 0; i < m_Results.size(); ++i)
	{
		const IVertex::SP & spVertex = m_Results[i];
		if (! spVertex)
			continue;

		const IVertex::EdgeList & out = spVertex->GetOutEdges();
		for(size_t k=0;k<out.size();++k)
		{
			if (! m_spCondition || m_spCondition->Test( out[k]->GetProperties() ) )
				results.push_back( out[k]->GetDestination() );
		}
	}

	m_Results.swap( results );
}

//----------------------------------------------

void InTraverser::BuildGremlinQuery( std::string & a_Query, Json::Value & a_Bindings )
{
	a_Query += ".inE()";
	if ( m_spCondition )
	{
		std::string cond = GremlinCondition( m_spCondition, a_Bindings );
		if ( cond.size() > 0 )
			a_Query += "." + cond;
	}
	a_Query += ".outV()";
}

void InTraverser::OnLocalTraverse()
{
	VertextList results;
	for (size_t i = 0; i < m_Results.size(); ++i)
	{
		const IVertex::SP & spVertex = m_Results[i];
		if (! spVertex)
			continue;

		const IVertex::EdgeList & in = spVertex->GetInEdges();
		for(size_t k=0;k<in.size();++k)
		{
			if (! m_spCondition || m_spCondition->Test( in[k]->GetProperties() ) )
				results.push_back( in[k]->GetSource() );
		}
	}

	m_Results.swap( results );
}

//------------------------------------------------------
