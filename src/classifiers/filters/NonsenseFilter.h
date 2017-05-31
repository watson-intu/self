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


#ifndef SELF_NONSENSE_FILTER_H
#define SELF_NONSENSE_FILTER_H

#include "classifiers/TextClassifier.h"
#include "SelfLib.h"

//! This filter looks for the nonsense intent and will ignore text if it makes no sense
class SELF_API NonsenseFilter : public ITextClassifierProxy::IClassFilter
{
public:
	RTTI_DECL();

	NonsenseFilter();

	//! ISerialiable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);
	//! IClassFilter interface
	virtual bool ApplyFilter(Json::Value & a_Intent);

	//! Mutators
	void SetActiveState(bool a_State);
	bool GetActiveState();

private:
	//! Methods
	void CalculateChatterConf();
	void IgnoreChatter();

	//! Data
	std::string		m_NonsenseClass;				// any text classified as this is ignored
	float			m_MinIgnoreNonsense;
	float 			m_DynamicNonsenseInterval;
	float 			m_DynamicNonsenseIntervalIncrement;
	bool			m_bActive;
};

#endif // SELF_NONSENSE_FILTER_H
