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


#include "UseSkillAction.h"
#include "skills/SkillManager.h"
#include "SelfInstance.h"

REG_SERIALIZABLE(UseSkillAction);
RTTI_IMPL(UseSkillAction, IAction);

void UseSkillAction::Serialize(Json::Value & json)
{
	IAction::Serialize(json);

	json["m_Skill"] = m_Skill;
	json["m_SkillParams"] = ISerializable::SerializeObject(&m_SkillParams);
	json["m_ReplaceParams"] = m_ReplaceParams;
}

void UseSkillAction::Deserialize(const Json::Value & json)
{
	IAction::Deserialize(json);

	m_Skill = json["m_Skill"].asString();
	ISerializable::DeserializeObject(json["m_SkillParams"], &m_SkillParams);
	m_ReplaceParams = json["m_ReplaceParams"].asBool();
}

void UseSkillAction::Execute(const Goal::SP & a_spGoal, Delegate<const State &> a_Callback)
{
	// merge our parameters into the goal parameters, this is done on purpose.
	new UseSkill( this, a_spGoal, a_Callback);
}

IAction * UseSkillAction::Clone()
{
	return new UseSkillAction(*this);
}

UseSkillAction::UseSkill::UseSkill(UseSkillAction * a_pAction, const Goal::SP & a_spGoal, Delegate<const IAction::State &> a_Callback) :
	m_pAction(a_pAction), m_spGoal( a_spGoal ), m_Callback(a_Callback)
{
	// merge our SkillParams into the one found in the goal object
	m_spGoal->GetParams().Merge(m_spGoal->GetParams().ResolveVariables(m_pAction->m_SkillParams.GetData()), m_pAction->m_ReplaceParams);
	// now resolve our skill name using the ParamsMap found in the Goal object
	std::string skill( m_spGoal->GetParams().ResolveVariables( m_pAction->m_Skill ) );
	// find and use our skill..
	SelfInstance::GetInstance()->GetSkillManager()->UseSkill(skill, m_spGoal->GetParams(),
		DELEGATE(UseSkill, OnSkillState, SkillInstance *, this), a_spGoal );
}

void UseSkillAction::UseSkill::OnSkillState(SkillInstance * a_pInstance)
{
	// translate skill state into action state and invoke the action state callback.
	IAction::State state;
	state.m_pAction = m_pAction;

	switch (a_pInstance->GetState())
	{
	case SkillInstance::US_COMPLETED:
		state.m_eState = IAction::AS_COMPLETED;
		break;
	case SkillInstance::US_FAILED:
		state.m_eState = IAction::AS_FAILED;
		break;
	case SkillInstance::US_UNAVAILABLE:
		state.m_eState = IAction::AS_UNAVAILABLE;
		break;
	case SkillInstance::US_ABORTED:
		state.m_eState = IAction::AS_ABORTED;
		break;
	case SkillInstance::US_FINDING:
	case SkillInstance::US_EXECUTING:
		state.m_eState = IAction::AS_EXECUTING;
		break;
	}

	if (m_Callback.IsValid())
		m_Callback(state);

	// destroy this object if this is our last callback..
	if (state.m_eState != IAction::AS_EXECUTING)
		delete this;
}
