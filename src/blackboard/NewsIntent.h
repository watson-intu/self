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


#ifndef SELF_NEWSINTENT_H
#define SELF_NEWSINTENT_H

#include "IIntent.h"

class SELF_API NewsIntent : public IIntent
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<NewsIntent>		SP;
	typedef boost::weak_ptr<NewsIntent>			WP;

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IIntent interface
	virtual void Create(const Json::Value & a_Intent, const Json::Value & a_Parse);

	const std::string & GetCompany() const
	{
		return m_Company;
	}

	void SetCompany(const std::string & a_Company)
	{
		m_Company = a_Company;
	}

private:
	//! Data
	std::string m_Company;
	Json::Value m_Parse;
};

#endif //SELF_WEATHERINTENT_H