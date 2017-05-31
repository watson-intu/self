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


#ifndef SELF_STATUS_H
#define SELF_STATUS_H

#include "IThing.h"

class SELF_API Status : public IThing
{
public:
	RTTI_DECL();

	//! Types
	enum StatusState
	{
		P_IDLE = 0x1,
		P_PROCESSING = 0x2,
		P_FINISHED = 0x4
	};

	typedef boost::shared_ptr<Status>       SP;
	typedef boost::weak_ptr<Status>         WP;

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Construction
	Status() :
		IThing(TT_MODEL, 300.0f), m_State(P_IDLE)
	{}

	Status(const std::string & a_ThingState) :
		IThing(TT_MODEL, 300.0f, a_ThingState ), m_State(P_IDLE)
	{}

	//! Accessors
	StatusState GetState()
	{
		return m_State;
	}

	void SetState(StatusState a_State)
	{
		m_State = a_State;

		switch (m_State)
		{
		case P_IDLE:
			IThing::SetState("IDLE");
			break;
		case P_PROCESSING:
			IThing::SetState("PROCESSING");
			break;
		case P_FINISHED:
			IThing::SetState("FINISHED");
			break;
		}
	}

private:
	//! Data
	StatusState m_State;
};

#endif //SELF_STATUS_H
