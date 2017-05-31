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


#ifndef SELF_IINTENT_H
#define SELF_IINTENT_H

#include "blackboard/IThing.h"
#include "utils/RTTI.h"

class SELF_API IIntent : public IThing
{
public:
    RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<IIntent>		SP;
	typedef boost::weak_ptr<IIntent>		WP;

    //! Construction
	IIntent( float a_fCondidence = 1.0f ) : m_Confidence(a_fCondidence), IThing( TT_COGNITIVE )
	{}
    virtual ~IIntent()
    {}

    //! ISerializable
    virtual void Serialize(Json::Value & json)
    {
		IThing::Serialize( json );
        json["m_Confidence"] = m_Confidence;
		json["m_Parse"] = m_Parse;
		json["m_Intent"] = m_Intent;
    }
    virtual void Deserialize(const Json::Value & json)
    {
		IThing::Deserialize( json );
        if( json.isMember("m_Confidence") )
            m_Confidence = json["m_Confidence"].asDouble();
		if (json["m_Intent"].isObject())
			m_Intent = json["m_Intent"];
		if (json["m_Parse"].isObject())
			m_Parse = json["m_Parse"];
    }

	double GetConfidence() const { return m_Confidence; }
	void SetConfidence( double a_Confidence ) { m_Confidence = a_Confidence; }

	virtual void Create(const Json::Value & a_Intent, const Json::Value & a_Parse);
protected:
    double		m_Confidence;
	Json::Value m_Parse;
	Json::Value m_Intent;
};

#endif //SELF_IINTENT_H
