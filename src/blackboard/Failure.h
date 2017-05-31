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


#ifndef SELF_FAILURE_H
#define SELF_FAILURE_H

#include "blackboard/IThing.h"
#include "utils/RTTI.h"

class SELF_API Failure : public IThing
{
public:
    RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<Failure>		SP;
	typedef boost::weak_ptr<Failure>		WP;

    //! Construction
	Failure() : IThing( TT_MODEL ), m_Confidence( 0.0 ), m_Threshold( 0.0 )
	{}
	Failure( const std::string & a_Name, const std::string & a_Info, double a_Confidence, double a_Threshold ) : 
		IThing( TT_MODEL ),
        m_Name(a_Name),
        m_Info(a_Info),
        m_Confidence(a_Confidence),
        m_Threshold(a_Threshold) 
	{}

    //! ISerializable
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

private:
    std::string     m_Name;
    std::string     m_Info;
    double          m_Confidence;
	double          m_Threshold;
};

#endif //SELF_FAILURE_H
