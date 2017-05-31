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


#ifndef LINEAR_SKILL_H
#define LINEAR_SKILL_H

#include <vector>

#include "ISkill.h"

//! This skill is composed of multiple skills running in sequence.
class SELF_API LinearSkill : public ISkill
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<LinearSkill>		SP;
	typedef boost::weak_ptr<LinearSkill>		WP;
	typedef std::vector< ISkill::SP >			SkillList;

	//! Construction
	LinearSkill();
	LinearSkill( const LinearSkill & a_Copy );
	virtual ~LinearSkill();

	const SkillList & GetSkillList() const 
	{
		return m_Skills;
	}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! ISkill interface
	virtual bool CanUseSkill();
	virtual void UseSkill( Delegate<ISkill *> a_Callback, const ParamsMap & a_Params);
	virtual bool AbortSkill();
	virtual ISkill * Clone();

	void AddSkill( const ISkill::SP & a_pSkill );
	void ClearSkillList();

private:
	//! Data
	SkillList m_Skills;

	//! State Data
	Delegate<ISkill *> m_Callback;
	ParamsMap m_Params;
	int m_NextSkill;

	//! Callback
	void OnSkillDone( ISkill * a_pSkill );
};

#endif
