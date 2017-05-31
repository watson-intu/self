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


#ifndef SELF_CONFIRM_H
#define SELF_CONFIRM_H

#include "IThing.h"

//! This object is placed on the blackboard when we need self to confirm an action
//! before we act. The FeedbackAgent will handle the positive/negative feedback that
//! will trigger the confirm or cancel responses of this object. 
class SELF_API Confirm : public IThing
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<Confirm>			SP;
	typedef boost::weak_ptr<Confirm>			WP;

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Construction
	Confirm() :  IThing( TT_COGNITIVE ), m_bConfirmed( false )
	{}
	Confirm( const std::string & a_ConfirmType ) :
			IThing( TT_COGNITIVE ),
			m_ConfirmType( a_ConfirmType ),
			m_bConfirmed( false )
	{}
	Confirm( const std::string & a_ConfirmType, const Json::Value & a_Info ) :
			IThing( TT_COGNITIVE ),
			m_ConfirmType( a_ConfirmType ),
			m_bConfirmed( false ),
			m_Info(a_Info)
	{}

	bool IsConfirmed() const 
	{
		return m_bConfirmed;
	}
	const Json::Value & GetInfo() const
	{
		return m_Info;
	}
	const std::string & GetConfirmType() const
	{
		return m_ConfirmType;
	}

	void OnConfirmed();
	void OnCancelled();
	
private:
	//! Data
	Json::Value     m_Info;
	bool            m_bConfirmed;
	std::string     m_ConfirmType;
};

#endif //SELF_CONFIRM_H
