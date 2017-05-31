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


#include "ParallelSkill.h"
#include "SkillManager.h"

#include "utils/Log.h"

REG_SERIALIZABLE(ParallelSkill);
RTTI_IMPL( ParallelSkill, ISkill);

ParallelSkill::ParallelSkill() : m_ActiveSkills(0)
{}

ParallelSkill::ParallelSkill(const ParallelSkill & a_Copy) : ISkill(a_Copy)
{
	for (SkillList::const_iterator iSkill = a_Copy.m_Skills.begin(); iSkill != a_Copy.m_Skills.end(); ++iSkill)
		m_Skills.push_back( ISkill::SP( (*iSkill)->Clone() ) );
}

ParallelSkill::~ParallelSkill()
{
	ClearSkillList();
}

void ParallelSkill::Serialize(Json::Value & json)
{
	ISkill::Serialize(json);

	int index = 0;
	for (SkillList::iterator iSkill = m_Skills.begin(); iSkill != m_Skills.end(); ++iSkill)
		json["m_Skills"][index++] = ISerializable::SerializeObject( (*iSkill).get() );
}

void ParallelSkill::Deserialize(const Json::Value & json)
{
	ISkill::Deserialize(json);

	ClearSkillList();

	const Json::Value & skills = json["m_Skills"];
	for (Json::ValueConstIterator iChild = skills.begin(); iChild != skills.end(); ++iChild)
		m_Skills.push_back( ISkill::SP( ISerializable::DeserializeObject<ISkill>(*iChild)) );
}

bool ParallelSkill::CanUseSkill()
{
	if ( m_ActiveSkills > 0 )
		return false;		// still pending skills.
	for (SkillList::iterator iSkill = m_Skills.begin(); iSkill != m_Skills.end(); ++iSkill)
		if (!(*iSkill)->CanUseSkill())
			return false;
	return true;
}

void ParallelSkill::UseSkill(Delegate<ISkill *> a_Callback, const ParamsMap & a_Params)
{
	if ( m_eState == ACTIVE || m_eState == BLOCKED )
		AbortSkill();

	m_eState = ACTIVE;
	m_Callback = a_Callback;
	m_ActiveSkills = 0;

	if ( m_Skills.size() > 0 )
	{
		for(size_t i=0;i<m_Skills.size();++i)
		{
			Log::Debug( "ParallelSkill", "Using skill %s", m_Skills[i]->GetSkillName().c_str() );
			m_Skills[i]->UseSkill( DELEGATE( ParallelSkill, OnSkillDone, ISkill *, this ), a_Params );
			m_ActiveSkills += 1;
		}
	}
	else
	{
		m_eState = COMPLETED;
		if ( m_Callback.IsValid() )
		{
			m_Callback( this );
			m_Callback.Reset();
		}
	}
}

bool ParallelSkill::AbortSkill()
{
	Log::Debug( "ParallelSkill", "AbortSkill() invoked." );
	for(size_t i=0;i<m_Skills.size();++i)
		if (! m_Skills[i]->AbortSkill() )
			return false;

	m_ActiveSkills = 0;
	m_eState = INACTIVE;
	return true;
}

ISkill * ParallelSkill::Clone()
{
	return new ParallelSkill(*this);
}

void ParallelSkill::ClearSkillList()
{
	m_Skills.clear();
}

void ParallelSkill::OnSkillDone(ISkill * a_pSkill)
{
	if (a_pSkill != NULL && a_pSkill->GetState() != ISkill::COMPLETED)
	{
		Log::Error("ParallelSkill", "Skill %s failed to complete, aborting parallel skill %s.", 
			a_pSkill->GetSkillName().c_str(), GetSkillName().c_str());
		AbortSkill();

		m_eState = FAILED;
		if ( m_Callback.IsValid() )
		{
			m_Callback( this );
			m_Callback.Reset();
		}
	}
	else if ( --m_ActiveSkills <= 0 )
	{
		m_eState = COMPLETED;
		if ( m_Callback.IsValid() )
		{
			m_Callback( this );
			m_Callback.Reset();
		}
	}
}

void ParallelSkill::AddSkill(const ISkill::SP & a_pSkill)
{
	m_Skills.push_back(a_pSkill);
}