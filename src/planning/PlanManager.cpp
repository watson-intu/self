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


#include "PlanManager.h"
#include "planning/actions/UseSkillAction.h"
#include "planning/conditions/GoalParamsConditon.h"
#include "utils/MD5.h"

REG_SERIALIZABLE(PlanManager);
RTTI_IMPL(PlanManager, ISerializable);

PlanManager::PlanManager() : m_bActive( false ), m_nPendingOps( 0 ), m_fMinPlanScore( 0.5f )
{}

PlanManager::~PlanManager()
{}

void PlanManager::Serialize(Json::Value & json)
{
	ISerializable::SerializeMap("m_Plans", m_Plans, json);
	json["m_fMinPlanScore"] = m_fMinPlanScore;
}

void PlanManager::Deserialize(const Json::Value & json)
{
	ISerializable::DeserializeMap("m_Plans", json, m_Plans, false);

	if ( json.isMember( "m_fMinPlanScore" ) )
		m_fMinPlanScore = json["m_fMinPlanScore"].asFloat();
}

bool PlanManager::Start()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;
	if ( m_bActive )
		return false;

	m_bActive = true;

	IGraph::SP spGraph = pInstance->GetKnowledgeGraph();
	assert( spGraph.get() != NULL );

	//! Load the plans into memory
	const std::string & dataPath = pInstance->GetStaticDataPath();
	const SelfInstance::FileList & plans = pInstance->GetPlanFiles();
	for (PlanFiles::const_iterator iFile = plans.begin(); iFile != plans.end(); ++iFile)
	{
		std::string planFile( dataPath + *iFile );
		if (ISerializable::DeserializeFromFile(planFile, this) == NULL) 
			Log::Error("PlanManager", "Failed to load plan file %s.", planFile.c_str());
	}

	//! Load the plans loaded into memory in turn into the graph
	for (PlanMap::iterator itr = m_Plans.begin(); itr != m_Plans.end(); ++itr) 
	{
		if ( itr->second->IsEnabled() )
			new SavePlan( this, itr->second );
	}

	return true;
}

bool PlanManager::Stop()
{
	if (! m_bActive )
		 return false;

	while( m_nPendingOps > 0 )
	{
		ThreadPool::Instance()->ProcessMainThread();
		boost::this_thread::sleep( boost::posix_time::milliseconds(5) );
	}

	m_bActive = false;
	return true;
}

//! Traverse the graph to figure out the best plan for the provided goal object
Plan::SP PlanManager::SelectPlan( const Goal::SP & a_spGoal )
{
	Plan::SP spBestPlan;

	// search the local DB for the best plan for the provided goal object..
	float fBestPlan = m_fMinPlanScore;
	for( PlanMap::iterator iPlan = m_Plans.begin(); iPlan != m_Plans.end(); ++iPlan )
	{
		float fPlanScore = iPlan->second->TestPreConditions( a_spGoal );
		if ( fPlanScore > fBestPlan )
		{
			spBestPlan = iPlan->second;
			fBestPlan = fPlanScore;
		}
	}

	return spBestPlan;
}

bool PlanManager::AddPlan( const Plan::SP & a_spPlan, bool a_bAddRemote /*= true */)
{
	if ( ! a_spPlan )
		return false;
	if (! a_spPlan->IsEnabled() )
		return false;

	m_Plans[a_spPlan->GetPlanId()] = a_spPlan;

	if (a_bAddRemote)
		new SavePlan( this, a_spPlan );

	return true;
}

bool PlanManager::DeletePlan( const Plan::SP & a_spPlan )
{
	if ( a_spPlan.get() == NULL )
		return false;

	IGraph::SP spGraph = SelfInstance::GetInstance()->GetKnowledgeGraph();
	spGraph->SetModel("world");

	ITraverser::SP spDelete = spGraph->CreateTraverser(EqualityCondition("planId", Logic::EQ, a_spPlan->GetPlanId()));
	if (!spDelete->Start(DELEGATE(PlanManager, OnDeletePlan, ITraverser::SP, this)))
		Log::Error("PlanManager", "Failed to traverse plans for deletion");

	m_Plans.erase( a_spPlan->GetPlanId() );
	return true;
}

bool PlanManager::DeletePlan( const std::string & a_ID )
{
	PlanMap::iterator iPlan = m_Plans.find(a_ID);
	if (iPlan == m_Plans.end())
		return false;

	return DeletePlan( iPlan->second );
}

PlanInstance::SP PlanManager::ExecutePlan( Goal::SP a_pGoal, StateCallback a_Callback)
{
	PlanInstance::SP spInstance( new PlanInstance( this, a_pGoal, a_Callback) );
	if ( spInstance->Start() )
		return spInstance;

	return PlanInstance::SP();
}

void PlanManager::DiscoverPlans()
{
	if ( m_nPendingOps == 0 )
	{
		m_nPendingOps += 1;

		IGraph::SP spGraph = SelfInstance::GetInstance()->GetKnowledgeGraph();
		spGraph->SetModel("world");

		//! Perform a traversal on the graph and get new plans into memory
		ITraverser::SP spTraverser = spGraph->CreateTraverser(LabelCondition("plans"));
		if (!spTraverser->Start(DELEGATE(PlanManager, OnPlans, ITraverser::SP, this)))
		{
			Log::Error("PlanManager", "Failed to traverse graph of others.");
			m_nPendingOps -= 1;
		}
	}
}

//! Iterate through and add all the remote plans discovered that do not exist in memory
void PlanManager::OnPlans(ITraverser::SP a_spTraverser)
{
	if (a_spTraverser->Size() > 0)
	{
		//! This should be the list of all plan vertices with the label plan_instance
		for(size_t i = 0; i < a_spTraverser->Size(); ++i)
		{
			IVertex::SP spVertex = a_spTraverser->GetResult(i);

			std::string planId = spVertex->ToJson()["planId"].asString();
			if ( m_Plans.find( planId ) == m_Plans.end() )
			{
				//! Load this plan in memory
				Log::Debug("PlanManager", "Found a new remote plan ID %s, adding to the graph", planId.c_str());
				AddPlan(Plan::SP(ISerializable::DeserializeObject<Plan>( (*spVertex)["data"].asString() )), false);
			}
		}
	}
	m_nPendingOps -= 1;
}

void PlanManager::OnDeletePlan(ITraverser::SP a_spTraverser)
{
	IGraph::SP spGraph = SelfInstance::GetInstance()->GetKnowledgeGraph();
	for (size_t i = 0; i < a_spTraverser->Size(); ++i)
		a_spTraverser->GetResult(i)->Drop();
}

//--------------------------------------------

PlanManager::SavePlan::SavePlan( PlanManager * a_pManager, const Plan::SP & a_spPlan ) : 
	m_spPlan( a_spPlan ), m_pManager( a_pManager ),
	m_Data( ISerializable::SerializeObject( a_spPlan.get() ) ),
	m_HashId( JsonHelpers::Hash( m_Data, "GUID_" ) )
{
	IGraph::SP spGraph = SelfInstance::GetInstance()->GetKnowledgeGraph();
	spGraph->SetModel( "world" );

	// search for an existing skill with the same hashId, if not found then create the vertex in our graph.
	ITraverser::SP spTraverser = spGraph->CreateTraverser( 
		LogicalCondition( Logic::AND, 
			LabelCondition( "plan" ), 
			EqualityCondition( "planId", Logic::EQ, m_spPlan->GetPlanId() ), 
			EqualityCondition( "hashId", Logic::EQ, m_HashId ) 
		) 
	);

	m_pManager->m_nPendingOps += 1;
	if (! spTraverser->Start( DELEGATE( SavePlan, OnFindPlan, ITraverser::SP, this ) ) )
	{
		Log::Error( "SkilLManager", "Failed to start traverser for adding a new plan." );
		m_pManager->m_nPendingOps -= 1;
		delete this;
	}
}

void PlanManager::SavePlan::OnFindPlan( ITraverser::SP a_spTraverser )
{
	// create the vertex if the plan isn't already in our graph..
	if ( a_spTraverser->Size() == 0 )
	{
		Json::Value plan;
		plan["data"] = m_Data.toStyledString();
		plan["planId"] = m_spPlan->GetPlanId();
		plan["hashId"] = m_HashId;

		Log::Status( "SkillManager", "Adding plan %s to the graph.", m_spPlan->GetPlanId().c_str() );
		a_spTraverser->GetGraph()->SetModel("world");
		a_spTraverser->GetGraph()->CreateVertex( "plan", plan );
	}

	m_pManager->m_nPendingOps -= 1;
	if ( m_pManager->m_nPendingOps == 0 )
		m_pManager->DiscoverPlans();

	delete this;			// free this request object
}
