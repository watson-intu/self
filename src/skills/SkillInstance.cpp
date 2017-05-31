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


#include <assert.h>

#include "blackboard/UsedSkill.h"
#include "SkillInstance.h"
#include "SkillManager.h"
#include "ISkill.h"

SkillInstance::SkillInstance( SkillManager * a_pManager, const std::string & a_Skill, const ParamsMap & a_Params,
	const Delegate<SkillInstance *> a_Callback, const IThing::SP & a_spParent ) :
	m_pManager( a_pManager ), m_Params( a_Params ), m_Callback( a_Callback ), m_spParent( a_spParent )
{
	m_pManager->AddInstance( this );
	m_eState = US_FINDING;

	IGraph::SP spGraph = SelfInstance::GetInstance()->GetKnowledgeGraph();
	spGraph->SetModel("world");

	// search our graph for all skills connected to the provided skill ID.. this will return N vertices, we will select a skill
	// with the best weight.
	ITraverser::SP spTraverser = SelfInstance::GetInstance()->GetKnowledgeGraph()->CreateTraverser( LabelCondition( "skill" ) )->
		Filter( EqualityCondition( "name", Logic::EQ, a_Skill ) );
	if (! spTraverser->Start( DELEGATE( SkillInstance, OnSelectSkill, ITraverser::SP, this ) ) )
	{
		m_eState = US_UNAVAILABLE;
		if ( m_Callback.IsValid() )
		{
			m_Callback( this );
			m_Callback.Reset();
		}

		// destroy this object, do nothing after this is called
		delete this;
	}
}

SkillInstance::~SkillInstance()
{
	m_pManager->RemoveInstance( this );
}

void SkillInstance::OnSkillState( ISkill * a_pSkill )
{
	assert( a_pSkill == m_spSkill.get() );

	m_eState = m_spSkill->GetState() == ISkill::COMPLETED ? US_COMPLETED : US_FAILED;
	if ( m_spParent )
		m_spParent->AddChild( IThing::SP( new UsedSkill( m_spSkill, m_eState == US_FAILED ) ) );

	if ( m_Callback.IsValid() )
	{
		m_Callback( this );
		m_Callback.Reset();
	}

	// destroy this instance after callback is complete..
	delete this;
}

void SkillInstance::OnSelectSkill( ITraverser::SP a_spTraverser )
{
	IVertex::SP spBestSkill;
	for(size_t i=0;i<a_spTraverser->Size();++i)
	{
		IVertex::SP spSkill = a_spTraverser->GetResults()[ i ];
		if (! spSkill )
			continue;
		if (! spBestSkill || spBestSkill->GetTime() < spSkill->GetTime() )
			spBestSkill = spSkill;
	}

	if ( spBestSkill )
	{
		m_spSkill = m_pManager->FindCachedSkill( (*spBestSkill)["name"].asString() );
		if (! m_spSkill )
		{
			m_spSkill = ISkill::SP( ISerializable::DeserializeObject<ISkill>( (*spBestSkill)["data"].asString() ) );
			m_pManager->CacheSkill( m_spSkill );
		}
	}

	if ( m_spSkill )
	{
		m_eState = US_EXECUTING;
		m_spSkill->UseSkill( DELEGATE( SkillInstance, OnSkillState, ISkill *, this), m_Params );
	}
	else
	{
		// no skill found
		m_eState = US_UNAVAILABLE;
		if ( m_Callback.IsValid() )
		{
			m_Callback( this );
			m_Callback.Reset();
		}

		// destroy this object, do nothing after this is called
		delete this;
	}
}

bool SkillInstance::Abort()
{
	if( m_eState == US_EXECUTING && m_spSkill )
	{
		if ( m_spSkill->AbortSkill() )
		{
			m_eState = US_ABORTED;
			return true;
		}
	}
	return false;
}

bool SkillInstance::IsSkill(const std::string & a_SkillName)
{
	if(m_spSkill && m_spSkill->GetSkillName().compare(a_SkillName) == 0 )
		return true;
	return false;
}

//! Converts the enum values to their string names
const char * SkillInstance::StateToString( SkillInstance::UseSkillState a_State )
{
	switch (a_State)
	{
	case US_FINDING:
		return "FINDING";
	case US_EXECUTING:
		return "EXECUTING";
	case US_COMPLETED:
		return "COMPLETED";
	case US_FAILED:
		return "FAILED";
	case US_UNAVAILABLE:
		return "UNAVAILBLE";
	case US_ABORTED:
		return "ABORTED";
	}

	return "INVALID";
}

SkillInstance::UseSkillState SkillInstance::StringToState( const std::string & a_State )
{
	for(size_t i=0;i<US_COUNT;++i)
		if ( StateToString( (UseSkillState)i) == a_State )
			return (UseSkillState)i;

	return US_INVALID;
}

