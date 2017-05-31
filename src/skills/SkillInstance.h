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


#ifndef SKILL_INSTANCE_H
#define SKILL_INSTANCE_H

#include "utils/ParamsMap.h"
#include "utils/Delegate.h"
#include "models/IGraph.h"
#include "blackboard/IThing.h"
#include "ISkill.h"
#include "SelfLib.h"		// include last always

class SkillManager;

//! This object is created to use a skill through the SkillManager.
class SELF_API SkillInstance
{
public:
	//! Types
	enum UseSkillState {
		US_INVALID = -1,
		US_FINDING,				// we are searching for the skill remotely
		US_EXECUTING,			// we are executing the skill
		US_COMPLETED,
		US_FAILED,				// we failed to execute the skill
		US_UNAVAILABLE,			// the skill was not found
		US_ABORTED,				// the skill was aborted

		US_COUNT
	};

	//! Construction
	SkillInstance( SkillManager * a_pManager, const std::string & a_Skill, const ParamsMap & a_Params,
		const Delegate<SkillInstance *> a_Callback, const IThing::SP & a_spParent );
	~SkillInstance();

	const ISkill::SP & GetSkill() const
	{
		return m_spSkill;
	}
	UseSkillState GetState() const
	{
		return m_eState;
	}
	const ParamsMap & GetParams() const
	{
		return m_Params;
	}
	const IThing::SP & GetParent() const
	{
		return m_spParent;
	}
	bool IsSkill(const std::string & a_SkillName);


	bool Abort();

	//! Converts the enum values to their string names
	static const char * StateToString( UseSkillState a_State );
	static UseSkillState StringToState( const std::string & a_State );

private:
	//! Callback Handlers
	void OnSkillState( ISkill * a_pSkill );
	void OnSelectSkill( ITraverser::SP a_spTraverser );

	//! Data
	SkillManager *	m_pManager;
	ISkill::SP		m_spSkill;		// the pointer to the skill object, may be NULL if skill isn't found
	UseSkillState	m_eState;
	ParamsMap		m_Params;

	Delegate<SkillInstance *> 
					m_Callback;
	IThing::SP		m_spParent;
};

#endif
