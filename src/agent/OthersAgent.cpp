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


#include "OthersAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"

REG_SERIALIZABLE(OthersAgent);
RTTI_IMPL(OthersAgent, IAgent);

OthersAgent::OthersAgent()
{}

void OthersAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
}

void OthersAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
}

bool OthersAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert( pBlackboard != NULL );

	pBlackboard->SubscribeToType( "RecognizedFace", 
		DELEGATE( OthersAgent, OnRecognizedFace, const ThingEvent &, this ) );

	return true;
}

bool OthersAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert( pBlackboard != NULL );

	pBlackboard->UnsubscribeFromType( "RecognizedFace",  this );

	return true;
}

void OthersAgent::OnRecognizedFace(const ThingEvent & a_ThingEvent)
{
	SP spThis( boost::static_pointer_cast<OthersAgent>( shared_from_this() ) );
	new LoadRecognizedFace( spThis, a_ThingEvent.GetIThing() );
}

//-----------------------------------------------------------

OthersAgent::LoadRecognizedFace::LoadRecognizedFace( const SP & a_spAgent, const IThing::SP & a_spFace ) 
	: m_spAgent( a_spAgent ), m_spFace( a_spFace )
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );

	m_PersonId = (*a_spFace)["personId"].asString();
	if ( !m_PersonId.empty() && m_spAgent->m_Finding.find( m_PersonId ) == m_spAgent->m_Finding.end() )
	{
		m_spAgent->m_Finding.insert( m_PersonId );

		IGraph::SP spGraph = pInstance->GetKnowledgeGraph();
		spGraph->SetModel("others");

		ITraverser::SP spTraverser = spGraph->CreateTraverser( LogicalCondition( Logic::AND, 
			LabelCondition( "person" ), EqualityCondition( "personId", Logic::EQ, m_PersonId ) ) );
		if (! spTraverser->Start( DELEGATE( LoadRecognizedFace, OnPersonGraph, ITraverser::SP, this ) ) )
		{
			Log::Error( "LoadRecognizedFace", "Failed to traverse graph of others." );
			m_spAgent->m_Finding.erase( m_PersonId );
			delete this;
		}
	}
	else
		delete this;		// we are already searching for this face..
}

void OthersAgent::LoadRecognizedFace::OnPersonGraph( ITraverser::SP a_spTraverser )
{
	if ( a_spTraverser->Size() == 0 )		// no vertex found, add it..
	{
		IGraph::SP spGraph = SelfInstance::GetInstance()->GetKnowledgeGraph();
		assert( spGraph.get() != NULL );

		Log::Status( "Person", "Saving new person %s into graph", (*m_spFace)["personId"].asCString() );

		spGraph->SetModel( "others" );

		IGraph::PropertyMap props;
		props["personId"] = (*m_spFace)["personId"];
		props["data"] = ISerializable::SerializeObject( m_spFace.get() ).toStyledString();

		IVertex::SP spVertex = spGraph->CreateVertex( "person", props );
		m_spFace->SetVertex( spVertex );
	}
	else
	{
		m_spFace->SetVertex( a_spTraverser->GetResult(0) );
	}

	m_spAgent->m_Finding.erase( m_PersonId );
	delete this;
}

