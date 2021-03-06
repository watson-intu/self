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


#ifndef SKILL_MANAGER_H
#define SKILL_MANAGER_H

#include <string>
#include <map>
#include <set>

#include "SkillInstance.h"
#include "ISkill.h"

#include "utils/ParamsMap.h"
#include "utils/Delegate.h"
#include "utils/Factory.h"
#include "models/IGraph.h"
#include "SelfLib.h"				// include last

//! Forward declare
class SelfInstance;
class ISkill;

//! This manager manages all ISkill instances including the saving and loading of skills from local storage.
class SELF_API SkillManager
{
public:
	//! Types
	typedef std::list<std::string>					SkillFiles;
	typedef boost::shared_ptr<ISkill>				ISkillSP;
	typedef std::map< std::string, ISkillSP >		SkillMap;
	typedef std::set< SkillInstance * >				SkillSet;

	//! Constants
	static ISkillSP									NULL_SKILL;

	//! Construction
	SkillManager();
	~SkillManager();

	//! Accessors
	const SkillSet & GetActiveSkills() const;
	const ISkill::SP & FindCachedSkill( const std::string & a_GUID ) const;

	//! Use a skill by it's ID with the given parameters. The callback if provided
	//! will be invoked as the state changes for the skill. This object will be deleted
	//! automatically once the skill is completed after the final callback is made.
	void UseSkill( const std::string & a_skill, 
		const ParamsMap & a_Params,
		const Delegate<SkillInstance *> & a_Callback,
		const IThing::SP & a_spParent );
	void UseSkill( const std::string & a_skill, 
		const ParamsMap & a_Params,
		const Delegate<SkillInstance *> & a_Callback );
	void UseSkill( const std::string & a_skill, 
		const ParamsMap & a_Params,
		const IThing::SP & a_spParent );
	void UseSkill( const std::string & a_skill, 
		const IThing::SP & a_spParent );
	void UseSkill( const std::string & a_skill, 
		const ParamsMap & a_Params );
	void UseSkill( const std::string & a_skill );

	//! Abort all active skills
	bool AbortActiveSkills();
	bool AbortActiveSkill(const std::string & a_Skill);

	//! Start this manager
	bool Start();
	//! Stop this manager
	bool Stop();

	//! add a new skill into this manager. It will be saved into local storage and uploaded into remote storage.
	//! this manager takes ownership of this object and will delete it upon destruction.
	bool AddSkill( const ISkill::SP & a_spSkill );
	//! delete a skill object by it's pointer.
	bool DeleteSkill( const ISkill::SP & a_spSkill );
	//! delete all skills connected to the given name
	bool DeleteSkill( const std::string & a_Skill );

	void AddInstance( SkillInstance * a_pInstance );
	void RemoveInstance( SkillInstance * a_pInstance );
	//! Cache a skill by it's GUID
	void CacheSkill( const ISkill::SP & a_spSkill );

private:
	//! Types
	struct SaveSkill
	{
		ISkill::SP		m_spSkill;
		Json::Value		m_Data;
		std::string		m_HashId;

		SaveSkill( const ISkill::SP & a_spSkill );
		void OnFindSkill( ITraverser::SP a_spTraverser );
	};

	//! Data
	IVertex::SP		m_spSkills;
	SkillSet		m_ActiveSkills;
	SkillMap		m_SkillCache;

	void OnDeleteSkill( ITraverser::SP a_spTraveser );
	void OnSkillAdded( const Json::Value & );
    void OnSkillDeleted( const Json::Value & );
};

//------------------------------

inline const SkillManager::SkillSet & SkillManager::GetActiveSkills() const
{
	return m_ActiveSkills;
}

inline const ISkill::SP & SkillManager::FindCachedSkill( const std::string & a_GUID ) const
{
	SkillMap::const_iterator iSkill = m_SkillCache.find( a_GUID );
	if ( iSkill != m_SkillCache.end() )
		return iSkill->second;

	return NULL_SKILL;
}

inline void SkillManager::UseSkill( const std::string & a_skill, const ParamsMap & a_Params,
	const Delegate<SkillInstance *> & a_Callback,
	const IThing::SP & a_spParent )
{
	new SkillInstance( this, a_skill, a_Params, a_Callback, a_spParent );
}

inline void SkillManager::UseSkill( const std::string & a_skill, const ParamsMap & a_Params,
	const Delegate<SkillInstance *> & a_Callback )
{
	new SkillInstance( this, a_skill, a_Params, a_Callback, IThing::SP() );
}

inline void SkillManager::UseSkill( const std::string & a_skill, const ParamsMap & a_Params,
	const IThing::SP & a_spParent )
{
	new SkillInstance( this, a_skill, a_Params, Delegate<SkillInstance *>(), a_spParent );
}

inline void SkillManager::UseSkill( const std::string & a_skill, const IThing::SP & a_spParent )
{
	new SkillInstance( this, a_skill, ParamsMap(), Delegate<SkillInstance *>(), a_spParent );
}

inline void SkillManager::UseSkill( const std::string & a_skill, const ParamsMap & a_Params )
{
	new SkillInstance( this, a_skill, a_Params, Delegate<SkillInstance *>(), IThing::SP() );
}

inline void SkillManager::UseSkill( const std::string & a_skill )
{
	new SkillInstance( this, a_skill, ParamsMap(), Delegate<SkillInstance *>(), IThing::SP() );
}

inline bool SkillManager::AbortActiveSkills()
{
	for( SkillSet::iterator iSkill = m_ActiveSkills.begin(); 
	iSkill != m_ActiveSkills.end(); iSkill = m_ActiveSkills.begin() )
	{
		if (! (*iSkill)->Abort() )
			return false;		// failed to abort this skill

		delete *iSkill;
	}
	return true;
}

inline bool SkillManager::AbortActiveSkill(const std::string & a_Skill)
{
	for( SkillSet::iterator iSkill = m_ActiveSkills.begin(); iSkill != m_ActiveSkills.end(); ++iSkill )
	{
		if(!(*iSkill)->GetSkill()->GetSkillName().compare(a_Skill))
		{
			if(! (*iSkill)->Abort() )
				return false;

			delete *iSkill;
			return true;
		}
	}
	return false;
}

//---------------------

struct SELF_API SkillCollection : public ISerializable
{
	RTTI_DECL();

	typedef boost::shared_ptr<ISkill>				ISkillSP;
	typedef std::map< std::string, ISkillSP >		SkillMap;

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	SkillMap		m_SkillMap;
};

#endif
