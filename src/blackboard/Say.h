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


#ifndef SELF_SAY_H
#define SELF_SAY_H

#include "IThing.h"
#include "utils/StringUtil.h"

//! This object is placed on the blackboard when we need self to say anything
class SELF_API Say : public IThing
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<Say>			SP;
	typedef boost::weak_ptr<Say>			WP;

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Construction
	Say() : IThing( TT_COGNITIVE )
	{}
	Say( const std::string & a_Text ) :
		IThing( TT_COGNITIVE ), m_Text( a_Text )
	{}

	//! Accessors
	const std::string & GetText() const 
	{
		return m_Text;
	}

	void SetText( const std::string & a_Text )
	{
		m_Text = StringUtil::Trim( a_Text );
	}

private:
	//! Data
	std::string			m_Text;
};

#endif //SELF_IMAGE_H
