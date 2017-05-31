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


#ifndef SELF_PROXY_H
#define SELF_PROXY_H

#include "BlackBoard.h"
#include "IThing.h"

//! This thing links to another object on the blackboard.
class SELF_API Proxy : public IThing
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<Proxy>			SP;
	typedef boost::weak_ptr<Proxy>				WP;

	//! Construction
	Proxy( ThingCategory a_eType = TT_INVALID ) : IThing( a_eType )
	{}

	Proxy( ThingCategory a_eType, const IThing::SP & a_spThing ) : 
		IThing( a_eType, a_spThing->GetLifeSpan() )
	{
		if ( a_spThing )
		{
			m_TargetGUID = a_spThing->GetGUID();
			a_spThing->SetProxyID( GetGUID() );
		}
	}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Accessors
	const std::string &	GetTargetGUID() const
	{
		return m_TargetGUID;
	}
	IThing::SP GetTarget() 
	{
		if ( m_pBlackBoard != NULL )
			return m_pBlackBoard->FindThing( m_TargetGUID );
		return IThing::SP();
	}

private:
	//! Data
	std::string			m_TargetGUID;
};

#endif //SELF_IMAGE_H
