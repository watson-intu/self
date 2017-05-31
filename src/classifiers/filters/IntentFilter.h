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


#ifndef SELF_INTENT_FILTER_H
#define SELF_INTENT_FILTER_H

#include "classifiers/TextClassifier.h"
#include "SelfLib.h"

//! This filter will filter a response if it matches one of the provided intents
class SELF_API IntentFilter : public ITextClassifierProxy::IClassFilter
{
public:
	RTTI_DECL();

	IntentFilter();

	//! ISerialiable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);
	//! IClassFilter interface
	virtual bool ApplyFilter(Json::Value & a_Intent);

private:
	//! Data
	std::vector<std::string> m_Intents;
};

#endif // SELF_INTENT_FILTER_H
