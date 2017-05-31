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


#include "RequestAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/RequestIntent.h"
#include "blackboard/Say.h"
#include "blackboard/Goal.h"
#include "blackboard/Entity.h"
#include "blackboard/Image.h"

#include <fstream>

const std::string SKILL("skill");
const std::string VERB("verb");

REG_SERIALIZABLE(RequestAgent);
RTTI_IMPL( RequestAgent, IAgent );

RequestAgent::RequestAgent() : m_nPendingOps( 0 )
{}


void RequestAgent::Serialize( Json::Value & json )
{
	IAgent::Serialize( json );
	SerializeVector( "m_RequestFailedText", m_RequestFailedText, json );
	SerializeVector( "m_RequestFiles", m_RequestFiles, json );
	SerializeVector( "m_EntityFilter", m_EntityFilter, json );
}

void RequestAgent::Deserialize( const Json::Value & json )
{
	IAgent::Deserialize( json );
	DeserializeVector( "m_RequestFailedText", json, m_RequestFailedText );
	DeserializeVector( "m_RequestFiles", json, m_RequestFiles );
	DeserializeVector( "m_EntityFilter", json, m_EntityFilter );

	if ( m_RequestFiles.size() == 0 )
		m_RequestFiles.push_back( "shared/self_requests.json" );
	if ( m_RequestFailedText.size() == 0 )
		m_RequestFailedText.push_back( "I do not know how to %s." );
	if ( m_EntityFilter.size() == 0 )
	{
		m_EntityFilter.push_back( "verb" );
		m_EntityFilter.push_back( "noun" );
		m_EntityFilter.push_back( "directionPerpendicular" );
		m_EntityFilter.push_back( "directionParallel" );
	}
}

bool RequestAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;

	IGraph::SP spGraph = pInstance->GetKnowledgeGraph();
	assert(spGraph.get() != NULL);

	if ( m_RequestFiles.size() > 0 )
	{
		for ( size_t i=0;i<m_RequestFiles.size(); ++i)
		{
			std::ifstream input( (pInstance->GetStaticDataPath() + m_RequestFiles[i]).c_str() );
			if (input.is_open())
			{
				std::string json = std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
				input.close();

				Json::Value root;
				Json::Reader reader( Json::Features::strictMode() );
				if (!reader.parse(json, root))
				{
					Log::Error( "RequestAgent", "Failed to parse json: %s", reader.getFormattedErrorMessages().c_str() );
					continue;
				}

				for (Json::ValueIterator itr = root.begin(); itr != root.end(); ++itr)
					new SaveAction( this, *itr );
			}
		}
	}

	SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType( "RequestIntent",
		DELEGATE(RequestAgent, OnRequestIntent, const ThingEvent &, this), TE_ADDED);
	SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType( "RecognizedObject",
		DELEGATE(RequestAgent, OnRecognizedObject, const ThingEvent &, this), TE_ADDED);
	return true;
}

bool RequestAgent::OnStop()
{
	// wait for any pending ops to finish..
	while( m_nPendingOps > 0 )
	{
		ThreadPool::Instance()->ProcessMainThread();
		boost::this_thread::sleep( boost::posix_time::milliseconds(5) );
	}

	SelfInstance::GetInstance()->GetBlackBoard()->UnsubscribeFromType( "RequestIntent", this);
	SelfInstance::GetInstance()->GetBlackBoard()->UnsubscribeFromType( "RecognizedObject", this );
	return true;
}

void RequestAgent::OnRequestIntent(const ThingEvent & a_ThingEvent)
{
	RequestIntent::SP spIntent = DynamicCast<RequestIntent>(a_ThingEvent.GetIThing());
	assert( spIntent.get() != NULL );

	SP spThis( boost::static_pointer_cast<RequestAgent>( shared_from_this() ) );
	for (size_t i = 0; i < spIntent->GetRequests().size(); ++i)	
	{
		const RequestIntent::Request & req = spIntent->GetRequests()[i];
		new FindAction( spThis, spIntent, req.m_Verb, req.m_Target );
	}
}

void RequestAgent::OnRecognizedObject(const ThingEvent & a_ThingEvent)
{
	IThing::SP spRecognizedObject = a_ThingEvent.GetIThing();
	std::string objectId = (*spRecognizedObject)["objectId"].asString();
	m_RecognizedObjects[objectId] = spRecognizedObject;
}

void RequestAgent::OnGoalState( const ThingEvent & a_Event )
{
	if ( a_Event.GetThingEventType() == TE_STATE )
	{
		Goal::SP spGoal = DynamicCast<Goal>( a_Event.GetIThing() );
		if ( spGoal && spGoal->GetState() == "FAILED")
		{
			std::string verb(spGoal->GetParams()["verb"].asString());
			std::string speak = StringUtil::Format( m_RequestFailedText[ rand() % m_RequestFailedText.size() ].c_str(), verb.c_str() );
			spGoal->AddChild( Say::SP( new Say( speak ) ) );
		}
	}
}

//----------------------------------------------

RequestAgent::SaveAction::SaveAction( RequestAgent * a_pAgent, const Json::Value & a_Props ) : 
	m_pAgent( a_pAgent ), m_Props( a_Props )
{
	m_pAgent->m_nPendingOps += 1;

	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );
	IGraph::SP spGraph = pInstance->GetKnowledgeGraph();
	assert(spGraph.get() != NULL);

	spGraph->SetModel("world");

	std::string hashId = JsonHelpers::Hash( m_Props );
	m_Props["hashId"] = hashId;

	ITraverser::SP spTraverser = spGraph->CreateTraverser(LogicalCondition( Logic::AND,
		LabelCondition("action"), 
		EqualityCondition("hashId",Logic::EQ, hashId) ) );

	if (! spTraverser->Start( DELEGATE( SaveAction, OnTraverse, ITraverser::SP, this ) ) )
	{
		Log::Error( "RequestAgent", "Failed to traverse for action vertex." );
		m_pAgent->m_nPendingOps -= 1;
		delete this;
	}
}

void RequestAgent::SaveAction::OnTraverse( ITraverser::SP a_spTraverser )
{
	if ( a_spTraverser->Size() == 0 )
	{
		std::string skill( m_Props[SKILL].asString() );
		if (! skill.empty() )
		{
			Log::Status( "RequestAgent", "Adding new action %s to graph", skill.c_str() );
			a_spTraverser->GetGraph()->CreateVertex( "action", m_Props );
		}
	}
	m_pAgent->m_nPendingOps -= 1;
	delete this;
}

RequestAgent::FindAction::FindAction( const SP & a_spAgent,
	const RequestIntent::SP & a_spIntent, 
	const std::string & a_Verb, 
	const std::string & a_Noun ) : 
	m_spAgent( a_spAgent ), m_spIntent( a_spIntent ), m_Verb( a_Verb ), m_Noun( a_Noun )
{
	bool bFailed = true;

	const Json::Value & entities = a_spIntent->GetEntities();
	if ( entities.size() > 0 )
	{
		m_spAgent->m_nPendingOps += 1;

		IGraph::SP spGraph = SelfInstance::GetInstance()->GetKnowledgeGraph();
		assert( spGraph.get() != NULL );

		LogicalCondition cond( Logic::AND, LabelCondition("action") );
		for( Json::ValueConstIterator iEntity = entities.begin(); iEntity != entities.end(); ++iEntity )
		{
			std::string entity( (*iEntity)["entity"].asString() );
			if ( std::find( m_spAgent->m_EntityFilter.begin(), m_spAgent->m_EntityFilter.end(), entity) == m_spAgent->m_EntityFilter.end() )
				continue;		// ignored entity, skip

			EqualityCondition e( entity, Logic::EQ, (*iEntity)["value"] );
			cond.m_Conditions.push_back( IConditional::SP( e.Clone() ) );
			bFailed = false;
		}
	
		if (! bFailed )
		{
			spGraph->SetModel("world");
			ITraverser::SP spTraverser = spGraph->CreateTraverser( cond );

			if (!spTraverser->Start(DELEGATE(FindAction, OnTraverseRequest, ITraverser::SP, this)))
			{
				Log::Error("RequestIntent", "Failed to traverse graph of the model looking for Conversation intents.");
				m_spAgent->m_nPendingOps -= 1;
				delete this;
			}
		}
	}

	if ( bFailed )
	{
		// set the skill as the verb, we are learning a new skill in this case
		m_Skill = m_Verb;
		// no entities, so this is a new skill request..
		CreateGoal();
		delete this;
	}
}

void RequestAgent::FindAction::OnTraverseRequest( ITraverser::SP a_spTraverser )
{
	if ( a_spTraverser->Size() > 0 )
	{
		// TODO: look at all the vertexes and take the most recent one or best
		// weight based on feedback ..
		for(size_t i=0;i<a_spTraverser->Size();++i)
		{
			IVertex::SP spVertex = a_spTraverser->GetResult(i);
			if (! spVertex->GetProperties().isMember(SKILL) )
				continue;

			// the vertex will specify an exact verb to use for our target..
			m_Verb = (*spVertex)[VERB].asString();
			m_Skill = (*spVertex)[SKILL].asString();
			break;
		}
	}
	else
	{
		// failed to find a vertex, so set the skill as the verb and continue
		m_Skill = m_Verb;
	}

	CreateGoal();

	m_spAgent->m_nPendingOps -= 1;
	delete this;
}

void RequestAgent::FindAction::CreateGoal()
{
	Goal::SP spGoal(new Goal());

	ObjectMap::iterator iObject = m_spAgent->m_RecognizedObjects.find( m_Noun );
	if ( iObject != m_spAgent->m_RecognizedObjects.end())
	{
		IThing::SP spObject = iObject->second;

		Log::Debug("RequestAgent", "Performing complex request");

		spGoal->SetName(m_Verb);
		spGoal->GetParams()["intent"] = "complex_request";
		spGoal->GetParams()["transform"] = (*spObject)["transform"];
		spGoal->GetParams()["rotation"] = (*spObject)["rotation"];
		spGoal->GetParams()["name"] = (*spObject)["objectId"].asString();
		spGoal->GetParams()["object_guid"] = spObject->GetGUID();
	}
	else
	{
		spGoal->SetName(m_Verb);
		spGoal->GetParams()["intent"] = "request";
	}
	spGoal->GetParams()["target"] = m_Noun;
	spGoal->GetParams()["skill"] = m_Skill;
	spGoal->GetParams()["verb"] = m_Verb;

	Log::Status( "RequestAgent", "Adding request goal %s with %s", m_Skill.c_str(), m_Noun.c_str() );
	spGoal->Subscribe( DELEGATE( RequestAgent, OnGoalState, const ThingEvent &, m_spAgent ) );
	m_spIntent->AddChild(spGoal);
}

