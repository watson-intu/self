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


#ifndef SELF_SKILLTEACHINGAGENT_H
#define SELF_SKILLTEACHINGAGENT_H

#include "IAgent.h"
#include "blackboard/LearningIntent.h"
#include "blackboard/Text.h"
#include "blackboard/Confirm.h"
#include "blackboard/UsedSkill.h"
#include "blackboard/Goal.h"
#include "utils/Factory.h"

#include "SelfLib.h"


class SELF_API SkillTeachingAgent : public IAgent
{
public:
	RTTI_DECL();

	SkillTeachingAgent();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Callbacks
	void OnGoal(const ThingEvent & a_ThingEvent);
	void OnSkillUsed( const ThingEvent & a_ThingEvent );
	void OnText(const ThingEvent & a_ThingEvent);
	void OnLearningIntent(const ThingEvent & a_ThingEvent);
	void OnLearnSkill( LearningIntent::SP a_spIntent );
	void OnForgetSkill( LearningIntent::SP a_spIntent );
	void OnRetrainNLC(LearningIntent::SP a_spIntent, 
		const std::string & a_Class );
	void OnConfirmRetrain(const ThingEvent & a_ThingEvent);

	//! Data
	Goal::SP					m_spLastFailedGoal;
	std::list< UsedSkill::SP >	m_UsedSkills;

	std::vector<std::string>	m_LearningResponses;
	std::vector<std::string>	m_ForgetResponses;
	std::vector<std::string>	m_ConfirmRetrain;
	std::vector<std::string>	m_RetrainResponses;
	std::list<Text::SP>			m_TextHistory;
	bool						m_bEnableSkillTraining;
	bool						m_bEnableNLCTraining;
};

#endif //SELF_SKILLTEACHINGAGENT_H
