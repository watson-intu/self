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


#include "PlanInstance.h"
#include "PlanManager.h"

PlanInstance::PlanInstance( PlanManager * a_pManager, Goal::SP a_spGoal, StateCallback a_Callback ) :
	m_InstanceId( UniqueID().Get() ),
	m_eState(INVALID),
	m_pManager( a_pManager ),
	m_spGoal( a_spGoal ),
	m_StateCallback( a_Callback )
{
}

PlanInstance::~PlanInstance()
{
	//Log::Debug( "PlanInstance", "Destroyed." );
}

bool PlanInstance::Start()
{
	ChangeState(FINDING);

	// TODO: Look for a remote & local plan, for now we just look for a local plan.
	Plan::SP spPlan = m_pManager->SelectPlan( m_spGoal );
	if ( spPlan )
	{
		// grab a pointer to ourselves, this keeps us in memory while we are handling callbacks. 
		m_spThis = shared_from_this();
		// start the local plan..
		StartPlan( spPlan );
		return true;
	}
	else
	{
		// TODO: Normally, if we don't find a local plan we would look for a remote plan
		// that interface doesn't exist yet, so just go into the UNAVAILBLE state 
		//Log::Debug ( "PlanInstance", "No local plan found for goal %s.", m_spGoal->GetName().c_str() );
		ChangeState( UNAVAILABLE );
		return false;
	}
}

void PlanInstance::StartPlan( Plan::SP a_spPlan )
{
	Log::Debug("PlanInstance", "Starting plan, InstanceId: %s, PlanId: %s, Goal: %s, GoalType: %s", 
		m_InstanceId.c_str(), a_spPlan->GetPlanId().c_str(),
		m_spGoal->GetGUID().c_str(), m_spGoal->GetRTTI().GetName().c_str() );

	m_spPlan = a_spPlan;
	for(size_t i=0;i<m_spPlan->GetActions().size();++i)
		m_Actions.push_back( m_spPlan->GetActions()[i] );

	ChangeState( EXECUTING );
	NextAction();	
}

void PlanInstance::NextAction()
{
	if ( m_Actions.size() > 0 )
	{
		IAction::SP spAction = m_Actions.front();
		m_Actions.pop_front();

		if ( spAction->TestPreConditions( m_spGoal ) )
		{
			m_ActiveActions.insert(spAction);

			spAction->Execute( m_spGoal, DELEGATE( PlanInstance, OnActionResult, const IAction::State &, this ) );

			// if this action is non-blocking, go ahead and start the next action as well..
			if ( (spAction->GetActionFlags() & IAction::NON_BLOCKING) != 0 )
				NextAction();
		}
		else if ((spAction->GetActionFlags() & IAction::CAN_IGNORE_PRE) == 0)
		{
			Log::Debug("PlanInstance", "NextAction() - InstanceId: %s, pre-condition failed for action %s.",
				m_InstanceId.c_str(), spAction->GetActionId().c_str());
			ChangeState(FAILED);
		}
		else
			NextAction();
	}
	else if (m_ActiveActions.size() == 0)
	{
		// OK, all actions are completed.. test our post-conditions 
		if (m_spPlan->TestPostConditions(m_spGoal))
		{
			Log::Debug("PlanInstance", "... Plan Completed, InstanceId: %s", m_InstanceId.c_str());
			ChangeState(COMPLETED);
		}
		else
		{
			Log::Debug("PlanInstance", "InstanceId: %s, post-conditions failed for plan %s.",
				m_InstanceId.c_str(), m_spPlan->GetPlanId().c_str());
			ChangeState(FAILED);
		}
	}
}

void PlanInstance::OnActionResult( const IAction::State & a_ActionState )
{
	if (a_ActionState.m_eState != IAction::AS_EXECUTING )
	{
		m_ActiveActions.erase(a_ActionState.m_pAction->shared_from_this());
		if (m_eState == EXECUTING)
		{
			if ( a_ActionState.m_eState == IAction::AS_COMPLETED )
			{
				if (a_ActionState.m_pAction->TestPostConditions(m_spGoal))
				{
					NextAction();
				}
				else if ((a_ActionState.m_pAction->GetActionFlags() & IAction::CAN_IGNORE_POST) == 0)
				{
					Log::Debug("PlanInstance", "InstanceId: %s, Plan: %s, Action: %s, post-condition failed.",
						m_InstanceId.c_str(), m_spPlan->GetPlanId().c_str(), a_ActionState.m_pAction->GetActionId().c_str());
					ChangeState(FAILED);
				}
				else
					NextAction();
			}
			else if ((a_ActionState.m_pAction->GetActionFlags() & IAction::CAN_IGNORE_ERROR) == 0)
			{
				Log::Debug( "PlanInstance", "InstanceId: %s, Plan: %s, Action: %s, action failed.",
					m_InstanceId.c_str(), m_spPlan->GetPlanId().c_str(), a_ActionState.m_pAction->GetActionId().c_str());
				ChangeState(FAILED);
			}
			else
				NextAction();
		}
	}
}

void PlanInstance::ChangeState( PlanState a_eState )
{
	if ( m_eState != a_eState )
	{
		m_eState = a_eState;

		if (m_StateCallback.IsValid())
			m_StateCallback(shared_from_this());

		// if this plan is done and we have no outstanding callbacks, go ahead and release our this pointer.
		if ( (m_eState == FAILED || m_eState == ABORTED || m_eState == COMPLETED) && m_ActiveActions.size() == 0 )
			m_spThis.reset();
	}
}

const char * PlanInstance::GetPlanStateText(PlanState a_eState)
{
	static const char * TEXT[] =
	{
		"INVALID",
		"FINDING",					// we are searching for the plan remotely
		"EXECUTING",				// we are executing the plan
		"COMPLETED",				// plan is completed
		"FAILED",					// we failed to execute the plan
		"UNAVAILABLE",				// the plan was not found
		"ABORTED"					// the plan was aborted
	};

	int index = (int)a_eState;
	if (index < 0 || index >= sizeof(TEXT) / sizeof(TEXT[0]))
		return "?";
	return TEXT[index];
}
