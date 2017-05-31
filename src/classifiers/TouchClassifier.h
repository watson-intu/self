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


#ifndef SELF_TOUCHCLASSIFIER_H
#define SELF_TOUCHCLASSIFIER_H

#include "blackboard/Touch.h"
#include "blackboard/BlackBoard.h"
#include "SelfInstance.h"
#include "utils/Factory.h"
#include "SelfLib.h"
#include "IClassifier.h"

class SELF_API TouchClassifier : public IClassifier
{
public:
	RTTI_DECL();

	TouchClassifier()
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value &json);
	virtual void Deserialize(const Json::Value &json);

	//! IClassifier interface
	virtual const char *GetName() const;
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Callback handler
	void OnTouch(const ThingEvent &a_ThingEvent);
};

#endif
