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


#define _CRT_SECURE_NO_WARNINGS

#include "SelfGraph.h"
#include "SelfEdge.h"
#include "SelfVertex.h"
#include "SelfTraverser.h"

#include "utils/UniqueID.h"
#include "utils/IDataStore.h"
#include "utils/StringUtil.h"
#include "utils/Time.h"
#include "SelfInstance.h"

const double GRAPH_SAVE_INTERVAL = 30.0f;

REG_FACTORY( SelfGraph, IGraph::GetGraphFactory() );

RTTI_IMPL(SelfGraph, IGraph);
REG_SERIALIZABLE(SelfGraph);

//-----------------------------------

SelfGraph::SelfGraph() : 
	m_bLoading( false ),
	m_bLoaded( false ),
	m_bCreated( false ),
	m_bError( false ),
	m_pGraph( NULL ),
	m_pStorage( NULL ), 
	m_nPendingOps( 0 )
{}

SelfGraph::SelfGraph( const std::string & a_GraphId ) : IGraph( a_GraphId ),
	m_bLoading( false ),
	m_bLoaded( false ),
	m_bCreated( false ),
	m_bError( false ),
	m_pGraph( NULL ),
	m_pStorage( NULL ), 
	m_nPendingOps( 0 )
{}

SelfGraph::~SelfGraph()
{
	Close();
}

void SelfGraph::Serialize(Json::Value & json)
{
	json["m_GraphId"] = m_GraphId;

	int i =0;
	for( VertexMap::const_iterator iVertex = m_VertexMap.begin(); iVertex != m_VertexMap.end(); ++iVertex )
		json["m_Verts"][i++] = ISerializable::SerializeObject( iVertex->second.get() );
	i = 0;
	for( EdgeMap::const_iterator iEdge = m_EdgeMap.begin(); iEdge != m_EdgeMap.end(); ++iEdge )
		json["m_Edges"][i++] = ISerializable::SerializeObject( iEdge->second.get() );
}

void SelfGraph::Deserialize(const Json::Value & json)
{
	m_VertexMap.clear();
	m_EdgeMap.clear();

	if (json.isMember("m_GraphId"))
		m_GraphId = json["m_GraphId"].asString();

	const Json::Value & verts = json["m_Verts"];
	for (size_t i = 0; i < verts.size(); ++i)
	{
		SelfVertex::SP spVertex( ISerializable::DeserializeObject<SelfVertex>( verts[i], new SelfVertex() ) );
		spVertex->SetGraph( this );

		m_VertexMap[ spVertex->GetId() ] = spVertex;
	}

	const Json::Value & edges = json["m_Edges"];
	for (size_t i = 0; i < edges.size(); ++i)
	{
		SelfEdge::SP spEdge( ISerializable::DeserializeObject<SelfEdge>( edges[i], new SelfEdge() ) );
		spEdge->SetGraph( this );

		VertexMap::iterator iSource = m_VertexMap.find( spEdge->GetSourceId() );
		VertexMap::iterator iDest = m_VertexMap.find( spEdge->GetDestinationId() );
		if ( iSource == m_VertexMap.end() || iDest == m_VertexMap.end() )
		{
			Log::Error( "SelfGraph", "Failed to connect edge %s", spEdge->GetId().c_str() );
			continue;
		}

		spEdge->SetSource( iSource->second );
		spEdge->SetDestination( iDest->second );

		m_EdgeMap[ spEdge->GetId() ] = spEdge;
	}
}

bool SelfGraph::Load( OnGraphLoaded a_Callback )
{
	if ( m_bLoading )
		return true;

	m_bLoading = true;
	m_OnGraphLoaded = a_Callback;

	m_pStorage = IDataStore::Create( m_GraphId, Json::Value() );
	if ( m_pStorage == NULL )
	{
		Log::Error( "SelfGraph", "Failed to create local data store %s", m_GraphId.c_str() );
		return false;
	}

	// load all vertexes first..
	m_pStorage->Load( "vertex_%", DELEGATE(SelfGraph, OnLoadVertex, Json::Value *, this ) );
	return true;
}

bool SelfGraph::Connect( OnGraphConnected a_Callback, const std::string & a_ServiceId /*= "GraphV1"*/ )
{
	if ( a_ServiceId.empty() || Config::Instance() == NULL )
		return false;
	if (! m_bLoaded )
		return false;

	m_bCreated = false;
	m_OnGraphConnected = a_Callback;

	m_pGraph = Config::Instance()->FindService<Graph>( a_ServiceId );
	if ( m_pGraph == NULL )
	{
		Log::Warning( "SelfGraph", "Failed to find service %s", a_ServiceId.c_str() );
		return false;
	}

	if ( m_pGraph->GetSchema( m_GraphId ) == NULL )
	{
		Log::Status( "SelfGraph", "Creating remote graph %s", m_GraphId.c_str() );
		m_pGraph->CreateGraph( m_GraphId, DELEGATE( SelfGraph, OnGraphCreated, const Json::Value &, this) );

		while(! m_bCreated && !m_bError )
		{
			ThreadPool::Instance()->ProcessMainThread();
			boost::this_thread::sleep( boost::posix_time::milliseconds(5) );
		}

		if ( m_bError )
			return false;
	}

	DiscoverModels();
	return true;
}

bool SelfGraph::Import( const Json::Value & a_Import )
{
	return ISerializable::DeserializeObject( a_Import, this ) != NULL;
}

bool SelfGraph::Export( Json::Value & a_Export )
{
	a_Export = ISerializable::SerializeObject( this );
	return true;
}

void SelfGraph::Clear()
{
	if ( m_pStorage != NULL )
	{
		m_pStorage->Drop();
		m_pStorage->Start( m_GraphId, Json::Value() );
	}

	m_VertexMap.clear();
	m_EdgeMap.clear();

	// TODO: send gremlin command to wipe all vertexes
}

bool SelfGraph::Close()
{
	if ( !m_bLoaded )
		return false;

	m_bLoaded = false;
	m_bLoading = false;

	// wait for all pending operations to complete..
	while( m_nPendingOps > 0 )
	{
		ThreadPool::Instance()->ProcessMainThread();
		boost::this_thread::sleep( boost::posix_time::milliseconds(5));
	}

	if ( m_pStorage != NULL )
	{
		if (! m_pStorage->Stop() )
		{
			Log::Error( "SelfGraph", "Failed to stop storage." );
			return false;
		}

		delete m_pStorage;
		m_pStorage = NULL;
	}

	return true;
}

bool SelfGraph::Drop()
{
	if (! m_bLoaded )
		return false;
	
	m_bLoaded = false;
	m_bLoading = false;

	// wait for all pending operations to complete..
	while( m_nPendingOps > 0 )
	{
		ThreadPool::Instance()->ProcessMainThread();
		boost::this_thread::sleep( boost::posix_time::milliseconds(5));
	}

	if (m_pGraph != NULL )
		m_pGraph->DeleteGraph( m_GraphId, DELEGATE( SelfGraph, OnGraphDeleted, const Json::Value &, this ) );

	if ( m_pStorage != NULL )
	{
		m_pStorage->Drop();
		delete m_pStorage;
		m_pStorage = NULL;
	}

	m_VertexMap.clear();
	m_EdgeMap.clear();
	return true;
}

void SelfGraph::SetModel( const std::string & a_Group )
{
	m_ActiveModel.reset();

	GroupMap::iterator iGroup = m_Models.find( a_Group );
	if ( iGroup == m_Models.end() )
	{
		Json::Value props;
		props["modelId"] = a_Group;

		IVertex::SP spNewModel = CreateVertex( "model", props );
		m_Models[ a_Group ] = spNewModel;
		m_ActiveModel = spNewModel;

		Log::Status( "SelfGraph", "Created new model %s", a_Group.c_str() );
	}
	else
	{
		// use an existing model vertex..
		m_ActiveModel = iGroup->second;
	}
}

ITraverser::SP SelfGraph::CreateTraverser(const Condition & a_spCond /*= Condition()*/)
{
	ISelfTraverser::SP spTraverser(new FilterTraverser(a_spCond));
	spTraverser->SetGraph(this);

	return spTraverser;
}

ITraverser::SP SelfGraph::LoadTraverser( const Json::Value & a_Data )
{
	ISelfTraverser::SP spTraverser( ISerializable::DeserializeObject<ISelfTraverser>( a_Data ) );
	spTraverser->SetGraph( this );

	return spTraverser;
}

IVertex::SP SelfGraph::CreateVertex( const std::string & a_Label, const PropertyMap & a_Properties, VertexEvent a_Callback /*= VertexEvent()*/ )
{
	SelfVertex::SP spNewVertex( new SelfVertex() );
	spNewVertex->SetGraph( this );
	if ( a_Callback.IsValid() )
		spNewVertex->GetNotificationList().Add( a_Callback );
	spNewVertex->SetLabel( a_Label );
	spNewVertex->SetProperties( a_Properties );
	spNewVertex->Save();

	// if we have an active model, then associate all vertexes with that given
	// model. This is done because titan DB requires an index to be able to search using gremlin.
	if ( m_ActiveModel )
		CreateEdge( "aspect", m_ActiveModel, spNewVertex, Json::Value() );

	return spNewVertex;
}

IVertex::SP SelfGraph::FindVertex(  const VertexId & a_Id )
{
	VertexMap::iterator iVertex = m_VertexMap.find( a_Id );
	if ( iVertex != m_VertexMap.end() )
		return iVertex->second;

	return IVertex::SP();
}

IEdge::SP SelfGraph::CreateEdge(
	const std::string & a_Label,
	const IVertex::SP & a_Src, 
	const IVertex::SP & a_Dest, 
	const PropertyMap & a_Properties,
	EdgeEvent a_Callback /*= EdgeEvent()*/ )
{
	SelfVertex::SP spSrc = DynamicCast<SelfVertex>( a_Src );
	SelfVertex::SP spDest = DynamicCast<SelfVertex>( a_Dest );
	if (! spSrc || !spDest )
		return IEdge::SP();

	SelfEdge::SP spNewEdge( new SelfEdge() );
	spNewEdge->SetGraph( this );
	if ( a_Callback.IsValid() )
		spNewEdge->GetNotificationList().Add( a_Callback );
	spNewEdge->SetLabel( a_Label );
	spNewEdge->SetSource( spSrc );
	spNewEdge->SetDestination( spDest );
	spNewEdge->SetProperties( a_Properties );
	spNewEdge->Save();

	return spNewEdge;
}

IEdge::SP SelfGraph::FindEdge(  const VertexId & a_Id )
{
	EdgeMap::iterator iEdge = m_EdgeMap.find( a_Id );
	if ( iEdge != m_EdgeMap.end() )
		return iEdge->second;

	return IEdge::SP();
}

std::string SelfGraph::ToString()
{
	return ISerializable::ToJson().toStyledString();
}

//--------------------------

void SelfGraph::OnGraphCreated( const Json::Value & a_Result )
{
	if ( a_Result.isNull() )
	{
		Log::Error( "SelfGraph", "Failed to create graph %s", m_GraphId.c_str() );
		m_bError = true;
	}
	else
	{
		Log::Status( "SelfGraph", "Graph %s created.", m_GraphId.c_str() );
		m_bCreated = true;
	}
}

void SelfGraph::OnGraphDeleted( const Json::Value & a_Result )
{
	if ( a_Result.isNull() )
		Log::Error( "SelfGraph", "Failed to delete graph %s", m_GraphId.c_str() );
}

void SelfGraph::OnLoadVertex(Json::Value * a_Json )
{
	bool bDone = (*a_Json)["_done"].asBool();
	if (! bDone )
	{
		std::string id = (*a_Json)["_id"].asString();
		if ( StringUtil::StartsWith( id, "vertex_" ) )
		{
			SelfVertex::SP spVertex( ISerializable::DeserializeObject<SelfVertex>( *a_Json, new SelfVertex() ) );
			spVertex->SetGraph( this );

			m_VertexMap[ spVertex->GetId() ] = spVertex;
			// TODO: Index properties for faster searching
		}
	}
	else
	{
		// load all edges now..
		m_pStorage->Load( "edge_%", DELEGATE( SelfGraph, OnLoadEdge, Json::Value *, this ) );
	}

	delete a_Json;
}

void SelfGraph::OnLoadEdge(Json::Value * a_Json )
{
	bool bDone = (*a_Json)["_done"].asBool();
	if (! bDone )
	{
		std::string id = (*a_Json)["_id"].asString();
		if ( StringUtil::StartsWith( id, "edge_" ) )
		{
			SelfEdge::SP spEdge( ISerializable::DeserializeObject<SelfEdge>( *a_Json, new SelfEdge() ) );
			spEdge->SetGraph( this );

			m_EdgeMap[ spEdge->GetId() ] = spEdge;
			// TODO: Index properties for faster searching
		}
	}
	else
	{
		OnLoadDone();
	}

	delete a_Json;
}

void SelfGraph::OnLoadDone()
{
	// enumerate all edges, connect them to their actual vertexes
	for( EdgeMap::iterator iEdge = m_EdgeMap.begin(); 
		iEdge != m_EdgeMap.end(); ++iEdge )
	{
		SelfEdge::SP spEdge = iEdge->second;
		VertexMap::iterator iSource = m_VertexMap.find( spEdge->GetSourceId() );
		VertexMap::iterator iDest = m_VertexMap.find( spEdge->GetDestinationId() );
		if ( iSource == m_VertexMap.end() || iDest == m_VertexMap.end() )
			continue;

		spEdge->SetSource( iSource->second );
		spEdge->SetDestination( iDest->second );
	}

	DiscoverModels();

	// we are done loading..!
	m_bLoaded = true;
	Log::Status( "SelfGraph", "Graph %s loaded (%u verts, %u edges local)", 
		m_GraphId.c_str(), m_VertexMap.size(), m_EdgeMap.size() );

	if ( m_OnGraphLoaded.IsValid() )
		m_OnGraphLoaded( shared_from_this() );
}

void SelfGraph::DiscoverModels()
{
	// discover all active models
	ITraverser::SP spFindGroups = CreateTraverser( LabelCondition("model") );
	if ( ! spFindGroups->Start( DELEGATE( SelfGraph, OnDiscoverModels, ITraverser::SP, this ) ) )
		Log::Error( "SelfGraph", "Failed to discover model." );
}

void SelfGraph::OnDiscoverModels( ITraverser::SP a_spTraverser )
{
	for(size_t i=0;i<a_spTraverser->Size();++i)
	{
		IVertex::SP spModelVertex = a_spTraverser->GetResult( i );
		std::string modelId( spModelVertex->GetProperty("modelId").asString() );
		m_Models[ modelId ] = spModelVertex;
	}

	//! Select a default model..
	SetModel( "world" );

	if ( m_OnGraphConnected.IsValid() )
		m_OnGraphConnected( shared_from_this() );
}

