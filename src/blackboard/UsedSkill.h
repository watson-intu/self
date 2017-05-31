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


#ifndef SELF_USE_SKILL_H
#define SELF_USE_SKILL_H

#include "blackboard/IThing.h"
#include "skills/ISkill.h"

class SkillInstance;

//! This object is placed on the blackboard when a skill is used.
class SELF_API UsedSkill : public IThing
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<UsedSkill>		SP;
	typedef boost::weak_ptr<UsedSkill>			WP;

	//! Construction
	UsedSkill() : IThing( TT_COGNITIVE )
	{}
	UsedSkill( const ISkill::SP & a_spSkill, bool a_bFailed ) : 
		IThing( TT_COGNITIVE ),
		m_spSkill( a_spSkill ),
		m_bFailed( a_bFailed )
	{}

	//! ISerializable interface
	virtual void Serialize( Json::Value & json );
	virtual void Deserialize( const Json::Value & json );

	//! Accessors
	const ISkill::SP & GetSkill() const
	{
		return m_spSkill;
	}
	bool IsFailed() const
	{
		return m_bFailed;
	}

protected:
	//! Data
	ISkill::SP		m_spSkill;
	bool			m_bFailed;
};

#endif //SELF_IINTENT_H
