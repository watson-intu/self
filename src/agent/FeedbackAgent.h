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


#ifndef SELF_FEEDBACKAGENT_H
#define SELF_FEEDBACKAGENT_H

#include "IAgent.h"
#include "blackboard/Confirm.h"
#include "blackboard/IIntent.h"
#include "utils/Factory.h"
#include "utils/TimerPool.h"

#include "SelfLib.h"

class SELF_API FeedbackAgent : public IAgent
{
public:
    RTTI_DECL();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Callbacks
	void OnLearningIntent(const ThingEvent & a_ThingEvent);
	void OnIntents(const ThingEvent & a_ThingEvent);
	void OnConfirm(const ThingEvent & a_ThingEvent);

	//! Data
	std::list<Confirm::SP>		m_Confirmations;
	IIntent::SP					m_spLastIntent;

	std::vector<std::string>	m_PositiveResponses;
	std::vector<std::string>	m_NegativeResponses;
	std::vector<std::string>	m_QuestionPositiveResponses;
	std::vector<std::string>	m_QuestionNegativeResponses;
};


#endif //SELF_FEEDBACKAGENT_H
