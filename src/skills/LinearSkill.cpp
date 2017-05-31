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


#include "LinearSkill.h"
#include "SkillManager.h"

#include "utils/Log.h"

REG_SERIALIZABLE(LinearSkill);
RTTI_IMPL( LinearSkill, ISkill);

LinearSkill::LinearSkill() : m_NextSkill(0)
{}

LinearSkill::LinearSkill(const LinearSkill & a_Copy) : ISkill(a_Copy)
{
	for (SkillList::const_iterator iSkill = a_Copy.m_Skills.begin(); iSkill != a_Copy.m_Skills.end(); ++iSkill)
		m_Skills.push_back( ISkill::SP( (*iSkill)->Clone()) );
}

LinearSkill::~LinearSkill()
{
	ClearSkillList();
}

void LinearSkill::Serialize(Json::Value & json)
{
	ISkill::Serialize(json);

	int index = 0;
	for (SkillList::iterator iSkill = m_Skills.begin(); iSkill != m_Skills.end(); ++iSkill)
		json["m_Skills"][index++] = ISerializable::SerializeObject((*iSkill).get() );
}

void LinearSkill::Deserialize(const Json::Value & json)
{
	ISkill::Deserialize(json);

	ClearSkillList();

	const Json::Value & skills = json["m_Skills"];
	for (Json::ValueConstIterator iChild = skills.begin(); iChild != skills.end(); ++iChild)
		m_Skills.push_back( ISkill::SP( ISerializable::DeserializeObject<ISkill>(*iChild)) );
}

bool LinearSkill::CanUseSkill()
{
	for(size_t i=0;i<m_Skills.size();++i)
		if ( !m_Skills[i]->CanUseSkill() )
			return false;
	return true;
}

void LinearSkill::UseSkill(Delegate<ISkill *> a_Callback, const ParamsMap & a_Params)
{
	if ( m_eState == ACTIVE || m_eState == BLOCKED )
		AbortSkill();

	m_Callback = a_Callback;
	m_Params = a_Params;
	m_eState = ACTIVE;
	m_NextSkill = 0;

	// invoke our callback manually to start using the first skill
	OnSkillDone( NULL );
}

bool LinearSkill::AbortSkill()
{
	Log::Debug( "LinearSkill", "AbortSkill() invoked." );
	for(size_t i=0;i<m_Skills.size();++i)
	{
		if (! m_Skills[i]->AbortSkill() )
			return false;
	}
	m_eState = INACTIVE;
	return true;
}

ISkill * LinearSkill::Clone()
{
	return new LinearSkill(*this);
}

void LinearSkill::AddSkill( const ISkill::SP & a_pSkill )
{
	m_Skills.push_back( a_pSkill );
}

void LinearSkill::ClearSkillList()
{
	m_Skills.clear();
}


void LinearSkill::OnSkillDone(ISkill * a_pSkill)
{
	if (a_pSkill != NULL && a_pSkill->GetState() != ISkill::COMPLETED)
	{
		Log::Error("LinearSkill", "Skill %s failed to complete, aborting linear skill %s.", a_pSkill->GetSkillName().c_str(), GetSkillName().c_str());
		m_eState = FAILED;
		
		if ( m_Callback.IsValid()) 
		{
			m_Callback(this);
			m_Callback.Reset();
		}
	}
	else
	{
		if (m_NextSkill < (int)m_Skills.size())
		{
			Log::Debug( "LinearSkill", "Using skill %s", m_Skills[m_NextSkill]->GetSkillName().c_str() );
			m_Skills[m_NextSkill++]->UseSkill(DELEGATE(LinearSkill, OnSkillDone, ISkill *, this), m_Params);
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
}

