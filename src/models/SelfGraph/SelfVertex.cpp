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

#define MAX_RETRIES			(3)

#include "SelfVertex.h"
#include "SelfGraph.h"
#include "utils/IDataStore.h"

RTTI_IMPL( SelfVertex, IVertex );
REG_SERIALIZABLE( SelfVertex );

void SelfVertex::Serialize(Json::Value & json)
{
	json["m_Id"] = m_Id;
	json["m_Label"] = m_Label;
	json["m_fTime"] = m_fTime;
	if (!m_Properties.isNull())
		json["m_Properties"] = m_Properties;
}

void SelfVertex::Deserialize(const Json::Value & json)
{
	m_Id = json["m_Id"].asString();
	m_Label = json["m_Label"].asString();
	m_fTime = json["m_fTime"].asDouble();
	if (json.isMember("m_Properties"))
		m_Properties = json["m_Properties"];
}

IVertex::EdgeSP SelfVertex::CreateEdge( const std::string & a_Label, const IVertex::SP & a_Dst, const PropertyMap & a_Properties )
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	if ( pGraph == NULL )
		return IVertex::EdgeSP();

	return pGraph->CreateEdge( a_Label, shared_from_this(), a_Dst, a_Properties );
}

bool SelfVertex::Save()
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	if ( pGraph == NULL )
		return false;
	if ( m_bDropped )
		return false;

	SaveLocal();

	// save changes into the graph service first..
	if ( pGraph->m_pGraph != NULL )
	{
		if ( m_Id[0] == LOCAL_ID_CHAR )
		{
			// make sure we don't try to create the same vertex multiple times
			if ( !m_bCreated )
			{
				m_bCreated = true;
				m_bUpdated = false;
				m_spThis = shared_from_this();

				if ( pGraph->m_pGraph->Createvertex( pGraph->m_GraphId,
					m_Label, m_Properties, DELEGATE( SelfVertex, OnVertexCreated, const Json::Value &, shared_from_this() ) ) )
				{
					pGraph->m_nPendingOps += 1;
				}
			}
			else
			{
				// we are still trying to create the vertex, but Save() has been invoked
				// again so we need to Save() the vertex once it's done with creation
				m_bUpdated = true;
			}
		}
		else
		{
			// update existing vertex
			m_spThis = shared_from_this();
			m_bUpdated = false;		// clear any updated flag

			if ( pGraph->m_pGraph->UpdateVertex( pGraph->m_GraphId,
				m_Id, m_Properties, DELEGATE( SelfVertex, OnVertexUpdated, const Json::Value &, shared_from_this() ) ) )
			{
				pGraph->m_nPendingOps += 1;
			}
		}
	}
	else if (! m_bCreated )
	{
		m_bCreated = true;
		m_NotificationList.Invoke( VertexEvent(IVertex::E_COMMITTED,shared_from_this()) );
	}
	
	return true;
}


bool SelfVertex::Drop()
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	if ( pGraph == NULL )
		return false;

	if (! m_bDropped )
		m_NotificationList.Invoke( VertexEvent(IVertex::E_DROPPING, shared_from_this() ) );

	// flag this vertex as dropped, since we may already have an asynchronous operation
	// ongoing to add this vertex into the graph.
	m_bDropped = true;

	// remove all edges connected to this vertex as well..
	while( m_InEdges.size() > 0 )
		m_InEdges[m_InEdges.size() - 1]->Drop();
	while( m_OutEdges.size() > 0 )
		m_OutEdges[m_OutEdges.size() - 1]->Drop();

	// delete from the cloud
	if ( pGraph->m_pGraph != NULL && m_Id[0] != LOCAL_ID_CHAR )
	{
		if ( pGraph->m_pGraph->DeleteVertex( pGraph->m_GraphId, m_Id,
			DELEGATE( SelfVertex, OnVertexDeleted, const Json::Value &, shared_from_this() ) ) )
		{
			pGraph->m_nPendingOps += 1;
		}
	}

	DeleteLocal();
	return true;
}

void SelfVertex::OnVertexCreated( const Json::Value & a_Result )
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	assert( pGraph != NULL );

	bool bRetry = false;
	if (! a_Result.isNull() )
	{
		std::string id = a_Result["result"]["data"][0]["id"].asString();
		assert( id.size() > 0 );

		DeleteLocal();		// remove data from old ID

		m_Id = id;
		m_nRetries = 0;

		m_NotificationList.Invoke( VertexEvent(IVertex::E_COMMITTED,shared_from_this()) );

		Log::Debug( "SelfVertex", "Vertex %s created.", m_Id.c_str() );
		if (! m_bDropped )
		{
			// if Save() was invoked while we were creating the vertex,
			// go ahead and update the vertex again.
			if ( m_bUpdated )
				Save();
			else
				SaveLocal();			// just save local changes such as the ID change

			for(size_t i=0;i<m_OutEdges.size();++i)
			{
				SelfEdge::SP spEdge = DynamicCast<SelfEdge>( m_OutEdges[i] );
				spEdge->SetSourceId( m_Id );
				spEdge->Save();
			}
			for(size_t i=0;i<m_InEdges.size();++i)
			{
				SelfEdge::SP spEdge = DynamicCast<SelfEdge>( m_InEdges[i] );
				spEdge->SetDestinationId( m_Id );
				spEdge->Save();
			}

		}
		else
			Drop();		// dropped during creation process, drop it again
	}
	else 
	{
		Log::Error( "SelfVertex", "Failed to create new vertex (%d/%d)", m_nRetries + 1, MAX_RETRIES );
		if ( ! m_bDropped && ++m_nRetries < MAX_RETRIES ) 
		{
			if ( pGraph->m_pGraph->Createvertex( pGraph->m_GraphId,
				m_Label, m_Properties, DELEGATE( SelfVertex, OnVertexCreated, const Json::Value &, shared_from_this() ) ) )
			{
				pGraph->m_nPendingOps += 1;
				bRetry = true;
			}
		}
	}

	pGraph->m_nPendingOps -= 1;
	if (! bRetry )
		m_spThis.reset();
}

void SelfVertex::OnVertexUpdated( const Json::Value & a_Result )
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	assert( pGraph != NULL );

	bool bRetry = false;
	if ( a_Result.isNull() )
	{
		Log::Error( "SelfVertex", "Failed to update vertex %s (%d/%d)", m_Id.c_str(), m_nRetries + 1, MAX_RETRIES );
		if ( ++m_nRetries < MAX_RETRIES )
		{
			if ( pGraph->m_pGraph->UpdateVertex( pGraph->m_GraphId,
				m_Id, m_Properties, DELEGATE( SelfVertex, OnVertexUpdated, const Json::Value &, shared_from_this() ) ) )
			{
				pGraph->m_nPendingOps += 1;
				bRetry = true;
			}
		}
	}
	
	pGraph->m_nPendingOps -= 1;
	if (! bRetry )
		m_spThis.reset();
}

void SelfVertex::OnVertexDeleted( const Json::Value & a_Result )
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	assert( pGraph != NULL );

	bool bRetry = false;
	if ( a_Result.isNull() )
	{
		Log::Error( "SelfVertex", "Failed to delete vertex %s (%d/%d)", m_Id.c_str(), m_nRetries + 1, MAX_RETRIES );
		if ( ++m_nRetries < MAX_RETRIES )
		{
			if ( pGraph->m_pGraph->DeleteVertex( pGraph->m_GraphId, m_Id,
				DELEGATE( SelfVertex, OnVertexDeleted, const Json::Value &, shared_from_this() ) ) )
			{
				pGraph->m_nPendingOps += 1;
				bRetry = true;
			}
		}
	}
	else
	{
		Log::Status( "SelfVertex", "Vertex %s deleted.", m_Id.c_str() );
		m_nRetries = 0;
	}

	pGraph->m_nPendingOps -= 1;
	if (! bRetry )
		m_spThis.reset();
}

void SelfVertex::SaveLocal()
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	assert( pGraph != NULL );

	m_fTime = Time().GetEpochTime();

	pGraph->m_VertexMap[ m_Id ] = shared_from_this();
	if ( pGraph->m_pStorage != NULL )
	{
		pGraph->m_pStorage->Save( "vertex_" + m_Id, 
			ISerializable::SerializeObject( this ) );
	}
}

void SelfVertex::DeleteLocal()
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	assert( pGraph != NULL );

	// delete from local storage
	if ( pGraph->m_pStorage != NULL )
		pGraph->m_pStorage->Delete( "vertex_" + m_Id );

	pGraph->m_VertexMap.erase( m_Id );
}

