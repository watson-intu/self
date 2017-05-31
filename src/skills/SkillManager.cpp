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
#include <fstream>

#include "utils/MD5.h"
#include "SelfInstance.h"
#include "SkillManager.h"
#include "ISkill.h"

SkillManager::ISkillSP SkillManager::NULL_SKILL;

SkillManager::SkillManager() 
{}

SkillManager::~SkillManager()
{}

bool SkillManager::Start()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;

	Log::Status("SkillManager", "SkillManager started.");

	const std::string & skillPath = SelfInstance::GetInstance()->GetStaticDataPath();

	// load the skill DB from local storage..
	const SelfInstance::FileList & skills = pInstance->GetSkillFiles();
	for( SelfInstance::FileList::const_iterator iFile = skills.begin(); iFile != skills.end(); ++iFile )
	{
		std::string skillFile( skillPath + *iFile );

		SkillCollection collection;
		if (ISerializable::DeserializeFromFile(skillFile, &collection) == NULL)
		{
			Log::Error("SkillManager", "Failed to load skills from %s.",skillFile.c_str());
			continue;
		}

		for( SkillMap::iterator iSkill = collection.m_SkillMap.begin();
			iSkill != collection.m_SkillMap.end(); ++iSkill )
		{
			const ISkill::SP & spSkill = iSkill->second;
			if ( spSkill->GetSkillName().size() == 0 )
				spSkill->SetSkillName( iSkill->first );		// use the key if no name was provided
			AddSkill( iSkill->second );
		}
	}

	return true;
}

bool SkillManager::Stop()
{
	return true;
}

bool SkillManager::AddSkill(const ISkill::SP & a_spSkill )
{
	if ( !a_spSkill)
		return false;
	if ( a_spSkill->GetSkillName().size() == 0 )
	{
		Log::Error( "SkillManager", "Skill is required to have a name." );
		return false;
	}
	if (! a_spSkill->IsEnabled() )
		return false;

	new SaveSkill( a_spSkill );
	return true;
}

bool SkillManager::DeleteSkill(const ISkill::SP & a_spSkill)
{
	if (!a_spSkill)
		return false;

	m_SkillCache.erase( a_spSkill->GetSkillName() );

	ITraverser::SP spDelete = SelfInstance::GetInstance()->GetKnowledgeGraph()->CreateTraverser( LabelCondition( "skill" ) )->
		Filter( EqualityCondition( "name", Logic::EQ, a_spSkill->GetSkillName() ) );
	if (! spDelete->Start( DELEGATE( SkillManager, OnDeleteSkill, ITraverser::SP, this ) ) )
		Log::Error( "SkillManager", "Failed to traverse skills for delete." );

	return true;
}

bool SkillManager::DeleteSkill(const std::string & a_Skill )
{
	if ( a_Skill.size() == 0 )
		return false;

	// remove any cached skill
	m_SkillCache.erase( a_Skill );

	ITraverser::SP spDelete = SelfInstance::GetInstance()->GetKnowledgeGraph()->CreateTraverser( LabelCondition( "skill" ) )->
		Filter( EqualityCondition( "name", Logic::EQ, a_Skill ) );
	if (! spDelete->Start( DELEGATE( SkillManager, OnDeleteSkill, ITraverser::SP, this ) ) )
		Log::Error( "SkillManager", "Failed to traverse skills for delete." );

	return true;
}

void SkillManager::AddInstance( SkillInstance * a_pInstance )
{
	m_ActiveSkills.insert( a_pInstance );
}

void SkillManager::RemoveInstance( SkillInstance * a_pInstance )
{
	m_ActiveSkills.erase( a_pInstance );
}

void SkillManager::CacheSkill( const ISkill::SP & a_spSkill )
{
	if ( a_spSkill )
		m_SkillCache[ a_spSkill->GetSkillName() ] = a_spSkill;
}

void SkillManager::OnDeleteSkill( ITraverser::SP a_spTraveser )
{
	IGraph::SP spGraph = SelfInstance::GetInstance()->GetKnowledgeGraph();
	for(size_t i=0;i<a_spTraveser->Size();++i)
		a_spTraveser->GetResults()[i]->Drop();
}

void SkillManager::OnSkillAdded( const Json::Value & response )
{
	if (! response.isNull() && response.isMember("status") && response["status"].asString() == "ok" )
		Log::Debug( "SkillManager", "Skill add success." );
	else
		Log::Error( "SkillManager", "Received null response in OnSkillAdded()." );
}

void SkillManager::OnSkillDeleted( const Json::Value & response)
{
	if(!response.isNull() && response.isMember("status") && response["status"].asString() == "ok" )
		Log::Debug( "SkillManager", "Skill deleted success." );
	else
		Log::Error("SkillManager", "Received null response in OnSkillDeleted()");
}

//--------------------------------------------

SkillManager::SaveSkill::SaveSkill( const ISkill::SP & a_spSkill ) : m_spSkill( a_spSkill ),
	m_Data( ISerializable::SerializeObject( a_spSkill.get() ) ),
	m_HashId( JsonHelpers::Hash( m_Data, "GUID_" ) )
{
	IGraph::SP spGraph = SelfInstance::GetInstance()->GetKnowledgeGraph();
	spGraph->SetModel( "world" );

	// search for an existing skill with the same hashId, if not found then create the vertex in our graph.
	ITraverser::SP spTraverser = spGraph->CreateTraverser(
		LogicalCondition( Logic::AND, 
			LabelCondition( "skill" ),
			EqualityCondition( "name", Logic::EQ, m_spSkill->GetSkillName() ),
			EqualityCondition( "hashId", Logic::EQ, m_HashId )
		)
	);
	if (! spTraverser->Start( DELEGATE( SaveSkill, OnFindSkill, ITraverser::SP, this ) ) )
		Log::Error( "SkilLManager", "Failed to start traverser for adding a new skill." );
}

void SkillManager::SaveSkill::OnFindSkill( ITraverser::SP a_spTraverser )
{
	// create the vertex if the skill isn't already in our graph..
	if ( a_spTraverser->Size() == 0 )
	{
		Json::Value skill;
		skill["data"] = m_Data.toStyledString();
		skill["name"] = m_spSkill->GetSkillName();
		skill["hashId"] = m_HashId;

		Log::Status( "SkillManager", "Adding skill %s to the graph.", m_spSkill->GetSkillName().c_str() );
		a_spTraverser->GetGraph()->SetModel("world");
		a_spTraverser->GetGraph()->CreateVertex( "skill", skill );
	}

	delete this;			// free this request object
}

//--------------------------------------------

RTTI_IMPL( SkillCollection, ISerializable );
REG_SERIALIZABLE( SkillCollection );

void SkillCollection::Serialize(Json::Value & json)
{
	ISerializable::SerializeMap("m_SkillMap", m_SkillMap, json);
}

void SkillCollection::Deserialize(const Json::Value & json)
{
	ISerializable::DeserializeMap("m_SkillMap", json, m_SkillMap, false );
}
