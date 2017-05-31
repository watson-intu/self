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


#ifndef SELF_PLAN_MANAGER_H
#define SELF_PLAN_MANAGER_H

#include <string>
#include <map>
#include <set>

#include "utils/ISerializable.h"
#include "PlanInstance.h"
#include "SelfLib.h"				// include last

//! This manager manages all ISkill instances including the saving and loading of skills from local storage.
class SELF_API PlanManager : public ISerializable
{
public:
	RTTI_DECL();

	//! Types
	typedef std::list<std::string>					PlanFiles;
	typedef std::map< std::string, Plan::SP >		PlanMap;
	typedef Delegate<PlanInstance::SP>				StateCallback;

	//! Construction
	PlanManager();
	~PlanManager();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Find a plan by it's unique ID.
	Plan * FindPlan( const std::string & a_ID ) const;

	//! Start this manager
	bool Start();
	//! Stop this manager
	bool Stop();

	//! Find a local plan based on the ability to complete the provided goal.
	Plan::SP SelectPlan( const Goal::SP & a_spGoal );
	//! Add a new plan into this manager, if a_bAddRemote is true then the plan is uploaded
	//! to the remote server as well.
	bool AddPlan( const Plan::SP & a_spPlan, bool a_bAddRemote = true );
	//! delete a plan object by it's pointer.
	bool DeletePlan( const Plan::SP & a_spPlan );
	//! delete a plan by it's ID
	bool DeletePlan( const std::string & a_ID );

	//! The main interface for executing a plan to complete the provided goal. This routine
	//! may generate a new plan to complete the given goal if no plans are found that are suitable
	//! for completing the given goal. The callback will be invoked each time the PlanInstance
	//! state is changed.
	PlanInstance::SP ExecutePlan( Goal::SP a_pGoal, StateCallback a_Callback );

	//! Invoke to discover any new plans not in the local map
	void DiscoverPlans();
	//! The callback to handle adding remote plans discovered through the graph traversal into the memory
	void OnPlans(ITraverser::SP a_spTraverser);

	//! The callback to handle deleting remote plans
	void OnDeletePlan(ITraverser::SP a_spTraverser);

private:
	struct SavePlan
	{
		PlanManager *	m_pManager;
		Plan::SP		m_spPlan;
		Json::Value		m_Data;
		std::string		m_HashId;

		SavePlan( PlanManager * a_pManager, const Plan::SP & a_spPlan );
		void OnFindPlan( ITraverser::SP a_spTraverser );
	};

	//! Data
	bool			m_bActive;
	volatile int	m_nPendingOps;

	//! Serialized Data
	PlanMap			m_Plans;
	float			m_fMinPlanScore;			// the minimum pre-condition score required for a local plan to be considered for selection.
};

//----------------------------------------------

inline Plan * PlanManager::FindPlan( const std::string & a_ID ) const
{
	PlanMap::const_iterator iPlan = m_Plans.find( a_ID );
	if ( iPlan != m_Plans.end() )
		return iPlan->second.get();

	return NULL;
}

#endif
