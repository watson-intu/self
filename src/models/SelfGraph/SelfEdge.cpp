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

#include "SelfEdge.h"
#include "SelfGraph.h"
#include "utils/IDataStore.h"

RTTI_IMPL( SelfEdge, IEdge );
REG_SERIALIZABLE( SelfEdge );

void SelfEdge::Serialize(Json::Value & json)
{
	json["m_Id"] = m_Id;
	json["m_SourceId"] = m_SourceId;
	json["m_DestinationId"] = m_DestinationId;
	json["m_Label"] = m_Label;
	json["m_fTime"] = m_fTime;
	if (!m_Properties.isNull())
		json["m_Properties"] = m_Properties;
}

void SelfEdge::Deserialize(const Json::Value & json)
{
	m_Id = json["m_Id"].asString();
	m_SourceId = json["m_SourceId"].asString();
	m_DestinationId = json["m_DestinationId"].asString();
	m_Label = json["m_Label"].asString();
	m_fTime = json["m_fTime"].asDouble();
	if (json.isMember("m_Properties"))
		m_Properties = json["m_Properties"];
}

bool SelfEdge::Save()
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	if ( pGraph == NULL )
		return false;
	if ( m_bDropped )
		return false;

	m_fTime = Time().GetEpochTime();

	SaveLocal();

	// save changes into the graph service next..
	if ( pGraph->m_pGraph != NULL )
	{
		if ( m_Id[0] == LOCAL_ID_CHAR )		
		{
			if ( !m_bCreated && m_SourceId[0] != LOCAL_ID_CHAR && m_DestinationId[0] != LOCAL_ID_CHAR )
			{
				m_bCreated = true;
				m_bUpdated = false;
				m_spThis = shared_from_this();

				if ( pGraph->m_pGraph->CreateEdge( pGraph->m_GraphId, 
					m_SourceId, m_DestinationId, m_Label, m_Properties,
					DELEGATE( SelfEdge, OnEdgeCreated, const Json::Value &, shared_from_this() ) ) )
				{
					pGraph->m_nPendingOps += 1;
				}
			}
			else if ( m_bCreated )
			{
				// we are still trying to create the edge, but Save() has been invoked
				// again so we need to Save() the edge once it's done with creation
				m_bUpdated = true;
			}
		}
		else
		{
			m_spThis = shared_from_this();
			m_bUpdated = false;

			if ( pGraph->m_pGraph->UpdateEdge( pGraph->m_GraphId,
				m_Id, m_Properties, DELEGATE( SelfEdge, OnEdgeUpdated, const Json::Value &, shared_from_this() ) ) )
			{
				pGraph->m_nPendingOps += 1;
			}
		}
	}
	else if (! m_bCreated )
	{
		m_bCreated = true;
		m_NotificationList.Invoke( EdgeEvent(IEdge::E_COMMITTED, shared_from_this()) );
	}

	return true;
}

bool SelfEdge::Drop()
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	if ( pGraph == NULL )
		return false;

	if(! m_bDropped )
		m_NotificationList.Invoke( EdgeEvent(IEdge::E_DROPPING,shared_from_this()) );
	m_bDropped = true;

	// detach this edge from our source and destination vertexes if possible..
	SelfVertex::SP spSource = DynamicCast<SelfVertex>( m_Source.lock() );
	if ( spSource )
		spSource->RemoveOutEdge( shared_from_this() );
	SelfVertex::SP spDest = DynamicCast<SelfVertex>( m_Destination.lock() );
	if ( spDest )
		spDest->RemoveInEdge( shared_from_this() );

	// delete from the cloud
	if ( pGraph->m_pGraph != NULL && m_Id[0] != LOCAL_ID_CHAR )
	{
		m_spThis = shared_from_this();

		if ( pGraph->m_pGraph->DeleteEdge( pGraph->m_GraphId, m_Id,
			DELEGATE( SelfEdge, OnEdgeDeleted, const Json::Value &, this ) ) )
		{
			pGraph->m_nPendingOps += 1;
		}
	}

	// delete from local storage
	DeleteLocal();
	return true;
}

bool SelfEdge::ResolveEdge()
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	assert( pGraph != NULL );

	IVertex::SP spSource = pGraph->FindVertex( m_SourceId );
	if (! spSource )
		return false;
	IVertex::SP spDest = pGraph->FindVertex( m_DestinationId );
	if (! spDest )
		return false;

	SetSource( spSource );
	SetDestination( spDest );
	return true;
}


void SelfEdge::OnEdgeCreated( const Json::Value & a_Result )
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	assert( pGraph != NULL );

	bool bRetry = false;
	if (!a_Result.isNull())
	{
		std::string id = a_Result["result"]["data"][0]["id"].asString();
		assert( id.size() > 0 );

		DeleteLocal();		// remove old data before changing the ID

		m_Id = id;
		m_nRetries = 0;

		m_NotificationList.Invoke( EdgeEvent(IEdge::E_COMMITTED, shared_from_this()) );

		Log::Debug( "SelfEdge", "Edge %s created.", m_Id.c_str() );
		if (! m_bDropped )
		{
			if ( m_bUpdated )
				Save();
			else
				SaveLocal();
		}
		else
			Drop();
	}
	else 
	{
		Log::Error( "SelfEdge", "Failed to create new edge (%d/%d)", m_nRetries + 1, MAX_RETRIES );
		if ( ! m_bDropped && ++m_nRetries < MAX_RETRIES ) 
		{
			if ( pGraph->m_pGraph->CreateEdge( pGraph->m_GraphId,
				m_SourceId, m_DestinationId, m_Label, m_Properties, 
				DELEGATE( SelfEdge, OnEdgeCreated, const Json::Value &, shared_from_this() ) ) )
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

void SelfEdge::OnEdgeUpdated( const Json::Value & a_Result )
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	assert( pGraph != NULL );

	bool bRetry = false;
	if ( a_Result.isNull() )
	{
		Log::Error( "SelfEdge", "Failed to update vertex %s (%d/%d)", m_Id.c_str(), m_nRetries + 1, MAX_RETRIES );
		if ( ++m_nRetries < MAX_RETRIES )
		{
			if ( pGraph->m_pGraph->UpdateEdge( pGraph->m_GraphId,
				m_Id, m_Properties, DELEGATE( SelfEdge, OnEdgeUpdated, const Json::Value &, shared_from_this() ) ) )
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

void SelfEdge::OnEdgeDeleted( const Json::Value & a_Result )
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	assert( pGraph != NULL );

	bool bRetry = false;
	if ( a_Result.isNull() )
	{
		Log::Error( "SelfEdge", "Failed to delete edge %s (%d/%d)", m_Id.c_str(), m_nRetries + 1, MAX_RETRIES );
		if ( ++m_nRetries < MAX_RETRIES )
		{
			if ( pGraph->m_pGraph->DeleteEdge( pGraph->m_GraphId, m_Id,
				DELEGATE( SelfEdge, OnEdgeDeleted, const Json::Value &, shared_from_this() ) ) )
			{
				pGraph->m_nPendingOps += 1;
				bRetry = true;
			}
		}
	}
	else
	{
		Log::Status( "SelfEdge", "Edge %s deleted.", m_Id.c_str() );
		m_nRetries = 0;
	}

	pGraph->m_nPendingOps -= 1;
	if (! bRetry )
		m_spThis.reset();
}

void SelfEdge::SaveLocal()
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	assert( pGraph != NULL );

	pGraph->m_EdgeMap[ m_Id ] = shared_from_this();
	if ( pGraph->m_pStorage != NULL )
	{
		pGraph->m_pStorage->Save( "edge_" + m_Id,
			ISerializable::SerializeObject( this ) );
	}
}

void SelfEdge::DeleteLocal()
{
	SelfGraph * pGraph = DynamicCast<SelfGraph>( m_pGraph );
	assert( pGraph != NULL );

	if ( pGraph->m_pStorage != NULL )
		pGraph->m_pStorage->Delete( "edge_" + m_Id );

	pGraph->m_EdgeMap.erase( m_Id );
}

