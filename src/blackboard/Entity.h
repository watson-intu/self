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


#ifndef SELF_ENTITY_H
#define SELF_ENTITY_H

#include "IThing.h"

//! An entity object classified from Visual Recognition
class SELF_API Entity : public IThing
{
public:
    RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<Entity>		SP;
	typedef boost::weak_ptr<Entity>			WP;

	Entity() : IThing( TT_PERCEPTION )
	{}

    const std::string & GetTopClass() const
    {
        return m_TopClass;
    }
	const double & GetTopScore() const
	{
		return m_TopScore;
	}
	const std::string & GetTypeHeirarchy() const
	{
		return m_TypeHierarchy;
	}
	
	void SetTopClass( const std::string & a_TopClass )
    {
        m_TopClass = a_TopClass;
    }

    void SetTopScore( const double & a_TopScore )
    {
        m_TopScore = a_TopScore;
    }
	void SetTypeHierarchy( const std::string & a_TypeHierarchy )
	{
		m_TypeHierarchy = a_TypeHierarchy;
	}

    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

private:
    //! Data
    std::string m_TopClass;
    double      m_TopScore;
	std::string	m_TypeHierarchy;
};

#endif //SELF_ENTITY_H
