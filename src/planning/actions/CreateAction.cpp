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


#include "CreateAction.h"

REG_SERIALIZABLE(CreateAction);
RTTI_IMPL(CreateAction, IAction);

CreateAction::CreateAction() : m_bAttachToRoot(false), m_fImportance(1.0f), m_ReplaceParams(true)
{}

void CreateAction::Serialize(Json::Value & json)
{
	IAction::Serialize(json);

	json["m_bAttachToRoot"] = m_bAttachToRoot;
	json["m_fImportance"] = m_fImportance;
	json["m_GoalParams"] = ISerializable::SerializeObject(&m_GoalParams, false);
	json["m_ReplaceParams"] = m_ReplaceParams;
	json["m_Object"] = m_Object;
	json["m_InitialState"] = m_InitialState;
	json["m_CompletedState"] = m_CompletedState;
	json["m_FailedState"] = m_FailedState;
}

void CreateAction::Deserialize(const Json::Value & json)
{
	IAction::Deserialize(json);

	if (json.isMember("m_bAttachToRoot"))
		m_bAttachToRoot = json["m_bAttachToRoot"].asBool();
	if (json.isMember("m_fImportance"))
		m_fImportance = json["m_fImportance"].asFloat();

	ISerializable::DeserializeObject(json["m_GoalParams"], &m_GoalParams);

	if (json.isMember("m_ReplaceParams"))
		m_ReplaceParams = json["m_ReplaceParams"].asBool();
	if (json.isMember("m_Object"))
		m_Object = json["m_Object"];
	if (json.isMember("m_InitialState"))
		m_InitialState = json["m_InitialState"].asString();
	if (json.isMember("m_CompletedState"))
		m_CompletedState = json["m_CompletedState"].asString();
	if (json.isMember("m_FailedState"))
		m_FailedState = json["m_FailedState"].asString();
}

void CreateAction::Execute(const Goal::SP & a_spGoal, Delegate<const State &> a_Callback)
{
	new CreateObject(this, a_spGoal, a_Callback);
}

IAction * CreateAction::Clone()
{
	return new CreateAction(*this);
}

CreateAction::CreateObject::CreateObject(CreateAction * a_pAction, Goal::SP a_spGoal, Delegate<const State &> a_Callback) :
	m_pAction(a_pAction), m_spGoal(a_spGoal), m_Callback(a_Callback)
{
	// firstly, merge the parameters from our action into the goal.
	m_spGoal->GetParams().Merge(m_pAction->m_GoalParams, m_pAction->m_ReplaceParams);

	IAction::State state;
	state.m_pAction = m_pAction;
	state.m_eState = IAction::AS_EXECUTING;
	if (m_Callback.IsValid())
		m_Callback(state);

	// resolve variables in our object so we can attempt to create an object.
	Json::Value json = m_spGoal->GetParams().ResolveVariables(m_pAction->m_Object);
	m_spObject = IThing::SP( ISerializable::DeserializeObject<IThing>(json) );
	if (m_spObject)
	{
		m_spObject->SetImportance(m_pAction->m_fImportance);
		if (m_pAction->m_InitialState.size() > 0)
			m_spObject->SetState(m_pAction->m_InitialState);

		bool bBlocking = m_pAction->m_CompletedState.size() > 0;
		if (bBlocking)
			m_spObject->Subscribe(DELEGATE(CreateObject, OnThingEvent, const ThingEvent &, this));

		if (!m_pAction->m_bAttachToRoot)
			m_spGoal->AddChild(m_spObject);
		else
			m_spGoal->GetBlackBoard()->AddThing(m_spObject);

		if (!bBlocking)
		{
			state.m_eState = IAction::AS_COMPLETED;
			if (m_Callback.IsValid())
				m_Callback(state);
		}
	}
	else
	{
		Log::Error("CreateAction", "Failed to deserialize object: %s", json.toStyledString().c_str());
		state.m_eState = IAction::AS_FAILED;
		if (m_Callback.IsValid())
			m_Callback(state);
	}

	if ( state.m_eState != IAction::AS_EXECUTING )
		delete this;
}

void CreateAction::CreateObject::OnThingEvent(const ThingEvent & a_Event)
{
	if (a_Event.GetThingEventType() == TE_STATE)
	{
		IAction::State state;
		state.m_pAction = m_pAction;

		if (a_Event.GetIThing()->GetState() == m_pAction->m_CompletedState)
		{
			Log::Debug("CreateAction", "IThing state %s, action completed.", a_Event.GetIThing()->GetState().c_str());
			state.m_eState = IAction::AS_COMPLETED;
		}
		else if (a_Event.GetIThing()->GetState() == m_pAction->m_FailedState)
		{
			Log::Debug("CreateAction", "IThing state %s, action failed!", a_Event.GetIThing()->GetState().c_str());
			state.m_eState = IAction::AS_FAILED;
		}
		else
		{
			Log::Debug("CreateAction", "IThing state %s, continuing execution.", a_Event.GetIThing()->GetState().c_str());
			state.m_eState = IAction::AS_EXECUTING;
		}

		if (m_Callback.IsValid())
			m_Callback(state);

		if (state.m_eState != IAction::AS_EXECUTING)
		{
			m_spObject->Unsubscribe(this);
			delete this;
		}
	}
}

