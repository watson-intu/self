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


#ifndef SELF_NAMEINTENT_H
#define SELF_NAMEINTENT_H

#include "IIntent.h"

class SELF_API NameIntent : public IIntent
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<NameIntent>		SP;
	typedef boost::weak_ptr<NameIntent>			WP;

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IIntent interface
	virtual void Create(const Json::Value & a_Intent, const Json::Value & a_Parse);

	const std::string & GetName() const
	{
		return m_Name;
	}

	void SetName(const std::string & a_Name)
	{
		m_Name = a_Name;
	}
	
	const std::string & GetPossessive() const
	{
		return m_Possessive;
	}

private:
	//! Data
	std::string m_Name;
	std::string m_Possessive;
	Json::Value m_Parse;
};

#endif //SELF_NAMEINTENT_H