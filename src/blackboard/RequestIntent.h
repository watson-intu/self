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


#ifndef SELF_REQUESTINTENT_H
#define SELF_REQUESTINTENT_H

#include "IIntent.h"

class SELF_API RequestIntent : public IIntent
{
public:
    RTTI_DECL();

    //! Types
	typedef boost::shared_ptr<RequestIntent>	SP;
	typedef boost::weak_ptr<RequestIntent>		WP;

	struct Request
	{
		Request()
		{}
		Request(const std::string & a_verb, const std::string & a_target) :
			m_Verb(a_verb), m_Target(a_target)
		{}
		std::string		m_Verb;
		std::string		m_Target;
	};
	typedef std::vector< Request >				RequestList;

	RequestIntent() : m_bGetAllItems( false )
	{}

   //! ISerializable interface
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

	//! IIntent interface
	virtual void Create(const Json::Value & a_Intent, const Json::Value & a_Parse);

    //! Accessors
	const std::string & GetText() const
	{
        return m_Text;
    }

	const Json::Value & GetTags() const		
	{
		return m_Tags;
	}

	const Json::Value & GetEntities() const
	{
		return m_Entities;
	}

	bool GetAllItems()
	{
		return m_bGetAllItems;
	}

	const std::string & GetType() const
	{
        return m_Type;
    }
	void AddRequest(const std::string & a_verb, const std::string & a_target)
	{
		Request newReq (a_verb, a_target);
		m_Requests.push_back(newReq);
	}


// NOTE: This will probably get removed eventually in favor of the GetTextParse() which is a
	// more complete representation of what was requested.
    const RequestList & GetRequests() const
	{
        return m_Requests;
    }

private:
    //! Data
    std::string m_Text;
	Json::Value m_Tags;
    std::string m_Type;
	RequestList	m_Requests;
	Json::Value m_Parse;
	Json::Value m_Entities;
	bool m_bGetAllItems;

};

#endif //SELF_REQUESTINTENT_H
