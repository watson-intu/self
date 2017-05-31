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


#ifndef SELF_QUESTIONAGENT_H
#define SELF_QUESTIONAGENT_H

#include "IAgent.h"
#include "utils/Factory.h"
#include "blackboard/QuestionIntent.h"
#include "blackboard/Confirm.h"
#include "SelfLib.h"

class SELF_API IQuestionAnswerProxy : public ISerializable,
	public boost::enable_shared_from_this<IQuestionAnswerProxy>
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<IQuestionAnswerProxy>		SP;
	typedef boost::weak_ptr<IQuestionAnswerProxy>		WP;

	//! Construction
	IQuestionAnswerProxy() : m_bPriorityFlag( true )
	{}
	virtual ~IQuestionAnswerProxy()
	{}

	//! ISerializable
	virtual void Serialize(Json::Value & json) 
	{
		json["m_bPriorityFlag"] = m_bPriorityFlag;
	}
	virtual void Deserialize(const Json::Value & json)
	{
		if ( json.isMember("m_bPriorityFlag") )
			m_bPriorityFlag = json["m_bPriorityFlag"].asBool();
	}

	//! Interface
	virtual void Start() = 0;
	virtual void Stop() = 0;
	virtual void AskQuestion( QuestionIntent::SP a_spQuestion, Delegate<const Json::Value &> a_Callback ) = 0;

	//! Accessors
	const bool HasPriority() const { return m_bPriorityFlag; }

private:
	//! Data
	bool	m_bPriorityFlag;
};

class SELF_API QuestionAgent : public IAgent
{
public:
	RTTI_DECL();

	//! Construction
	QuestionAgent();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);
	
	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

private:

	//! Types
	class ProcessQuestion
	{
	public:
		ProcessQuestion(QuestionAgent * a_pAgent, QuestionIntent::SP a_spQuestion);
		~ProcessQuestion();

	private:
		//! Callbacks
		void OnAnswer(const Json::Value & json);
		void OnFinalResult();

		QuestionAgent *		m_pAgent;
		QuestionIntent::SP	m_spQuestion;
		int					m_PendingRequests;
		int					m_CompletedRequests;
		bool				m_bGoalCreated;
		std::vector<Json::Value>	
							m_Responses;
	};

	//! Data
	double						m_MinDialogConfidence;
	double						m_MinAnswerConfidence;
	double						m_UseDialogConfidence;		
	int							m_nQuestionLimit;		// maximum number of active questions 

	std::vector<IQuestionAnswerProxy::SP>	
								m_QuestionAnswerProxies;				// one or more dialogs to query
	std::vector<std::string>	m_PipelineDownResponses;
	std::list<ProcessQuestion *>
								m_ActiveQuestions;

    void OnQuestionIntent(const ThingEvent & a_ThingEvent);
};

#endif //SELF_QUESTIONAGENT_H
