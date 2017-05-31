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


#ifndef SELF_VISUALTEACHINGAGENT_H
#define SELF_VISUALTEACHINGAGENT_H

#include "IAgent.h"
#include "blackboard/LearningIntent.h"
#include "blackboard/Image.h"
#include "blackboard/Entity.h"
#include "utils/Factory.h"

#include "SelfLib.h"


class SELF_API VisualTeachingAgent : public IAgent
{
public:
	RTTI_DECL();

	VisualTeachingAgent();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Callbacks
	void OnImage(const ThingEvent & a_ThingEvent);
	void OnLearningIntent(const ThingEvent & a_ThingEvent);
	void OnLearnObject( LearningIntent::SP a_spIntent );
	void OnForgetObject( LearningIntent::SP a_spIntent );

	//! Data
	std::vector<std::string>	m_LearnObjectResponses;
	std::vector<std::string>	m_ForgetObjectResponses;
	std::vector<std::string>    m_FailedImageTraining;
	std::list<Image::SP>		m_ImageHistory;
	bool						m_bEnableObjectTraining;
	unsigned int				m_TrainingImageCount;
};

#endif //SELF_VISUALTEACHINGAGENT_H
