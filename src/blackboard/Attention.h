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


#ifndef SELF_ATTENTION_H
#define SELF_ATTENTION_H

#include "IThing.h"

//! This object is placed on the blackboard when we need self to say anything
class SELF_API Attention : public IThing
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<Attention>			SP;
	typedef boost::weak_ptr<Attention>			WP;

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Construction
	Attention() : IThing(TT_PERCEPTION, 30.0f)
	{}
	Attention(double a_MinIntentConfidence, double a_MinMissNodeConfidence) :
		m_MinIntentConfidence(a_MinIntentConfidence), m_MinMissNodeConfidence(a_MinMissNodeConfidence), IThing(TT_PERCEPTION, 30.0f)
	{}

	//! Accessors
	double GetMinIntentConfidence() const
	{
		return m_MinIntentConfidence;
	}

	void SetMinIntentConfidence(double a_MinIntentConfidence)
	{
		m_MinIntentConfidence = a_MinIntentConfidence;
	}

	double GetMinMissNodeConfidence() const
	{
		return m_MinMissNodeConfidence;
	}

	void SetMinMissNodeConfidence(double a_MinMissNodeConfidence)
	{
		m_MinMissNodeConfidence = a_MinMissNodeConfidence;
	}

private:
	//! Data
	double m_MinIntentConfidence;
	double m_MinMissNodeConfidence;
};

#endif //SELF_ATTENTION_H

