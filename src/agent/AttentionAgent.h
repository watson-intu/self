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


#ifndef SELF_ATTENTIONAGENT_H
#define SELF_ATTENTIONAGENT_H

#include "IAgent.h"
#include "blackboard/Proximity.h"
#include "blackboard/Person.h"
#include "blackboard/HangOnIntent.h"
#include "utils/Factory.h"
#include "utils/TimerPool.h"
#include "sensors/SensorManager.h"
#include "sensors/GazeData.h"
#include "classifiers/ClassifierManager.h"
#include "classifiers/TextClassifier.h"
#include "classifiers/filters/NonsenseFilter.h"
#include "blackboard/HangOnIntent.h"

#include <dlib/mlp.h>

#include "SelfLib.h"

  
class SELF_API AttentionAgent : public IAgent
{ 
 public:
	RTTI_DECL();
	
AttentionAgent() :m_ElevatedThresh(0.0f), m_StandardThresh(0.0f),
                   m_LoweredThresh(0.0f), m_WaitTime( 5.0f ), m_MaxProximityDistance (1.0f),
				  m_bGesture( false ), m_bProximity( false )
		{}
	
	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);
	
	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

 private:

	typedef dlib::matrix<double, 2, 1> sample_type;
	sample_type sample;

	//! Data
	float           m_ElevatedThresh;
	float           m_LoweredThresh;
	float           m_StandardThresh;
	float			m_WaitTime;
	float 			m_MaxProximityDistance;

	bool 			m_bProximity;
	bool 			m_bGesture;
	
	TimerPool::ITimer::SP       m_spWaitTimer;
	TimerPool::ITimer::SP		m_spRefreshTimer;
	TimerPool::ITimer::SP		m_spActionTimer;

	void		OnGesture(const ThingEvent & a_ThingEvent);
	void		OnHoldOn(const ThingEvent & a_ThingEvent);
	void		OnProximity(const ThingEvent & a_ThingEvent);
	void 		OnText(const ThingEvent & a_ThingEvent);
	void        OnCheck();
	void 		OnRefresh();
	void 		OnAction();

};

#endif //SELF_ATTENTIONAGENT
