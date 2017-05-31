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


#ifndef SELF_ICONDITION_H
#define SELF_ICONDITION_H

#include "boost/enable_shared_from_this.hpp"
#include "boost/shared_ptr.hpp"
#include "jsoncpp/json/json.h"

#include "utils/ISerializable.h"
#include "utils/Logic.h"
#include "blackboard/Goal.h"
#include "SelfLib.h"

//! This is the base class for a condition object that can be attached to a plan as a post/pre condition.
class SELF_API ICondition : public ISerializable, public boost::enable_shared_from_this<ICondition>, public Logic
{
public:
	RTTI_DECL();

	//! Types 
	typedef boost::shared_ptr<ICondition>		SP;
	typedef boost::weak_ptr<ICondition>			WP;

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Test this condition, returns a value between 0.0 (false) and 1.0 (true). >= 0.5f is consider 
	//! a positive when a binary test is needed.
	virtual float Test( Goal::SP a_spGoal ) = 0;
	//! Make a new instance of this ICondition object.
	virtual ICondition * Clone() = 0;
};

#endif
