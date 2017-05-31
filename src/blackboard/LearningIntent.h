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


#ifndef SELF_LEARNINGINTENT_H
#define SELF_LEARNINGINTENT_H

#include "IIntent.h"

class SELF_API LearningIntent : public IIntent
{
public:
    RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<LearningIntent>		SP;
	typedef boost::weak_ptr<LearningIntent>			WP;

	LearningIntent()
	{}

    //! ISerializable interface
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

	//! IIntent interface
	virtual void Create(const Json::Value & a_Intent, const Json::Value & a_Parse);
	virtual void PublishObjectRecognized();

	//! Accessors and Mutators
	const std::string & GetText() const
	{
        return m_Text;
    }

	const Json::Value & GetTags() const 
	{
		return m_Tags;
	}

	const std::string & GetTarget() const
	{
        return m_Target;
    }

	const std::string & GetVerb() const
	{
        return m_Verb;
    }

	const std::string & GetAnswer() const
	{
		return m_Answer;
	}
	void SetTarget(const std::string & a_Target)
	{
		m_Target = a_Target;
	}

	void SetVerb(const std::string & a_Verb)
	{
		m_Verb = a_Verb;
	}

	const std::string & GetPhoneNumber() const
	{
		return m_PhoneNumber;
	}

private:
    //! Data
    std::string m_Text;
	Json::Value m_Tags;
    std::string m_Target;
    std::string m_Verb;
	std::string m_Answer;
	std::string m_PhoneNumber;
	Json::Value m_Parse;

	std::string FindTags( const std::string & a_Tag );
};

#endif //SELF_LEARNINGINTENT_H
