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


#ifndef PARALLEL_SKILL_H
#define PARALLEL_SKILL_H

#include <vector>

#include "ISkill.h"

//! This skill is composed of multiple skills running in sequence.
class SELF_API ParallelSkill : public ISkill
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<ParallelSkill>	SP;
	typedef boost::weak_ptr<ParallelSkill>		WP;
	typedef std::vector< ISkill::SP >			SkillList;

	//! Construction
	ParallelSkill();
	ParallelSkill( const ParallelSkill & a_Copy );
	virtual ~ParallelSkill();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! ISkill interface
	virtual bool CanUseSkill();
	virtual void UseSkill( Delegate<ISkill *> a_Callback, const ParamsMap & a_Params);
	virtual bool AbortSkill();
	virtual ISkill * Clone();

	void AddSkill( const ISkill::SP & a_pSkill);
	void ClearSkillList();

private:
	//! Data
	SkillList m_Skills;

	//! State Data
	Delegate<ISkill *> m_Callback;
	int m_ActiveSkills;

	//! Callback
	void OnSkillDone( ISkill * a_pSkill );
};

#endif
