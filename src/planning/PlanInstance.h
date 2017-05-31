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


#ifndef SELF_PLAN_INSTANCE_H
#define SELF_PLAN_INSTANCE_H

#include <set>
#include <list>

#include "boost/shared_ptr.hpp"
#include "boost/enable_shared_from_this.hpp"

#include "utils/ParamsMap.h"
#include "utils/Delegate.h"
#include "Plan.h"

#include "SelfLib.h"		// include last always

class PlanManager;

//! This object is created to use a skill through the SkillManager.
class SELF_API PlanInstance : public boost::enable_shared_from_this<PlanInstance>
{
public:
	//! Types
	enum PlanState {
		INVALID,
		FINDING,				// we are searching for the plan remotely
		EXECUTING,				// we are executing the plan
		COMPLETED,				// plan is completed
		FAILED,					// we failed to execute the plan
		UNAVAILABLE,			// the plan was not found
		ABORTED					// the plan was aborted
	};
	typedef boost::shared_ptr<PlanInstance>		SP;
	typedef boost::weak_ptr<PlanInstance>		WP;
	typedef Delegate<SP>						StateCallback;

	//! Construction
	PlanInstance( PlanManager * a_pManager, Goal::SP a_spGoal, StateCallback a_StateCallback );
	~PlanInstance();

	//! Accessors
	PlanManager *			GetManager() const;
	const std::string &		GetInstanceId() const;
	Goal::SP				GetGoal() const;
	const std::string &		GetPlanId() const;
	Plan::SP				GetPlan() const;
	PlanState				GetState() const;

	//! Mutators
	bool					Start();
	bool					Abort();			// attempt to stop this plan, returns true if abort worked

	//! Static
	static const char *		GetPlanStateText(PlanState a_eState);

private:
	void					StartPlan( Plan::SP a_spPlan );
	void					NextAction();
	void					OnActionResult( const IAction::State & a_Result );
	void					ChangeState( PlanState a_eState );

	//! Types
	typedef std::list<IAction::SP>		ActionList;
	typedef std::set<IAction::SP>		ActionSet;

	//! Data
	PlanManager *			m_pManager;
	std::string				m_InstanceId;
	Goal::SP				m_spGoal;
	Plan::SP				m_spPlan;
	PlanState				m_eState;
	ActionList				m_Actions;
	ActionSet				m_ActiveActions;
	StateCallback			m_StateCallback;

	SP						m_spThis;			// pointer to for keeping this object in memory while waiting for callbacks.
};

//----------------------------------------------------

inline PlanManager * PlanInstance::GetManager() const
{
	return m_pManager;
}

inline const std::string & PlanInstance::GetInstanceId() const
{
	return m_InstanceId;
}

inline Goal::SP PlanInstance::GetGoal() const
{
	return m_spGoal;
}

inline const std::string & PlanInstance::GetPlanId() const
{
	return m_spPlan ? m_spPlan->GetPlanId() : EMPTY_STRING;
}

inline Plan::SP PlanInstance::GetPlan() const
{
	return m_spPlan;
}

inline PlanInstance::PlanState PlanInstance::GetState() const
{
	return m_eState;
}


#endif
