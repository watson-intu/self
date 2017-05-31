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


#include "TelephonyAgent.h"
#include "blackboard/Goal.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/LearningIntent.h"
#include "blackboard/Say.h"
#include "sensors/TelephonyMicrophone.h"
#include "gestures/TelephonySpeechGesture.h"
#include "SelfInstance.h"
#include "gestures/GestureManager.h"

REG_SERIALIZABLE(TelephonyAgent);
RTTI_IMPL(TelephonyAgent, IAgent);

TelephonyAgent::TelephonyAgent() :
	m_AudioSent(0),
	m_AudioRecv(0),
	m_bInCall(false)
{}

void TelephonyAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
	json["m_TelephonyUtterance"] = m_TelephonyUtterance;
	json["m_LearnedPhoneNumberUtterance"] = m_LearnedPhoneNumberUtterance;
}

void TelephonyAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
	if (json.isMember("m_TelephonyUtterance"))
		m_TelephonyUtterance = json["m_TelephonyUtterance"].asString();
	if (json.isMember("m_LearnedPhoneNumberUtterance"))
		m_LearnedPhoneNumberUtterance = json["m_LearnedPhoneNumberUtterance"].asString();
}

void TelephonyAgent::BeginCall()
{
	if (!m_bInCall)
	{
		//! Start up telephony microphone
		m_pTelephonyMicrophone = TelephonyMicrophone::SP(new TelephonyMicrophone());
		SelfInstance::GetInstance()->GetSensorManager()->AddSensor(m_pTelephonyMicrophone, true);
		m_pTelephonySpeechGesture = TelephonySpeechGesture::SP(new TelephonySpeechGesture());
		SelfInstance::GetInstance()->GetGestureManager()->AddGesture(m_pTelephonySpeechGesture);

		m_bInCall = true;
	}
}

void TelephonyAgent::EndCall()
{
	if (m_bInCall)
	{
		SelfInstance::GetInstance()->GetSensorManager()->RemoveSensor(m_pTelephonyMicrophone);
		SelfInstance::GetInstance()->GetGestureManager()->RemoveGesture(m_pTelephonySpeechGesture);

		m_bInCall = false;
	}
}

bool TelephonyAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->SubscribeToType("TelephonyIntent",
		DELEGATE(TelephonyAgent, OnTelephonyIntent, const ThingEvent &, this), TE_ADDED);
	pInstance->GetBlackBoard()->SubscribeToType("Confirm",
		DELEGATE(TelephonyAgent, OnConfirm, const ThingEvent &, this), TE_STATE);
	pInstance->GetBlackBoard()->SubscribeToType("LearningIntent",
		DELEGATE(TelephonyAgent, OnLearnTelephoneNumber, const ThingEvent &, this), TE_ADDED);

	//! Find and start telephony service
	ITelephony * pTelephony = pInstance->FindService<ITelephony>();
	if (pTelephony == NULL)
	{
		Log::Error("TelephonyAgent", "ITelephony Service not available, not starting agent");
		return false;
	}

	pTelephony->Connect(pInstance->GetLocalConfig().m_EmbodimentId,
		DELEGATE(TelephonyAgent, OnCommand, const Json::Value &, this),
		DELEGATE(TelephonyAgent, OnAudioOut, const std::string &, this));

	return true;
}

bool TelephonyAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	EndCall();

	pInstance->GetBlackBoard()->UnsubscribeFromType("TelephonyIntent", this);
	pInstance->GetBlackBoard()->UnsubscribeFromType("Confirm", this);
	pInstance->GetBlackBoard()->UnsubscribeFromType("LearningIntent", this);

	ITelephony * pTelephony = pInstance->FindService<ITelephony>();
	if (pTelephony != NULL)
		pTelephony->Disconnect();

	return true;
}

void TelephonyAgent::OnTelephonyIntent(const ThingEvent & a_ThingEvent)
{
	TelephonyIntent::SP spTelephony = DynamicCast<TelephonyIntent>(a_ThingEvent.GetIThing());

	if (spTelephony)
	{
		if (!m_spActiveIntent || spTelephony->GetTelephonyAction() == "PROCESSING")
		{
			if (!m_ToNumber.empty())
				spTelephony->SetToNumber(m_ToNumber);

			ExecuteRequest(spTelephony);
		}
	}
}

void TelephonyAgent::ExecuteRequest(TelephonyIntent::SP a_pTelephony)
{
	Log::Debug("TelephonyAgent", "Telephony Intent object added to blackboard detected : action %s",
		a_pTelephony->GetTelephonyAction().c_str());

	ITelephony * pTelephony = Config::Instance()->FindService<ITelephony>();
	if (pTelephony != NULL)
	{
		bool isConfigured = Config::Instance()->IsConfigured(pTelephony->GetServiceId());

		if (!isConfigured)
		{
			Log::Error("TelephonyAgent", "Service not configured correctly!");
			a_pTelephony->AddChild(Goal::SP(new Goal("FailedOutgoingCall")));
		}
		else if (a_pTelephony->GetTelephonyAction() == "INITIATING") // creating a goal for making an outgoing call
		{
			m_spActiveIntent = a_pTelephony;

			Goal::SP spGoal(new Goal("OutgoingCall"));
			m_spActiveIntent->AddChild(spGoal);
		}
		else if (a_pTelephony->GetTelephonyAction() == "PROCESSING" && m_spActiveIntent)
			// proceed with the making the call with the call details in the intent object
		{
			if (!a_pTelephony->GetToNumber().empty())
			{
				Log::Status("TelephonyAgent", "Initiating call to %s", a_pTelephony->GetToNumber().c_str());
				m_spActiveIntent->SetState("PROCESSING");
				pTelephony->Dial(a_pTelephony->GetToNumber());
			}
			else
			{
				Log::Error("TelephonyAgent", "ITelephony service not available");
				m_spActiveIntent->SetState("ERROR");
				m_spActiveIntent.reset();
			}
		}
	}
}

void TelephonyAgent::OnCommand(const Json::Value & a_Call)
{
	std::string command = a_Call["command"].asString();

	if (a_Call.isMember("from_number"))
		m_FromNumber = a_Call["from_number"].asString();
	if (a_Call.isMember("to_number") && m_ToNumber.empty())
		m_ToNumber = a_Call["to_number"].asString();

	if (command == "receive_outgoing")
	{
		ITelephony * pTelephony = Config::Instance()->FindService<ITelephony>();
		if (pTelephony != NULL && pTelephony->Answer(m_FromNumber, m_ToNumber))
			BeginCall();
	}
	else if (command == "receive_incoming")
	{
		Goal::SP spGoal(new Goal("IncomingCall"));
		spGoal->GetParams()["number"] = m_FromNumber;

		SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spGoal);
	}
	else if (command == "hang_up")
	{
		EndCall();
		if (m_spActiveIntent)
		{
			Goal::SP spGoal(new Goal("EndingCall"));
			m_spActiveIntent->AddChild(spGoal);
			m_spActiveIntent->SetState("COMPLETED");
			m_spActiveIntent.reset();
		}
	}
}

void TelephonyAgent::OnAudioOut(const std::string & a_Data)
{
	m_AudioRecv += a_Data.size();

	m_pTelephonyMicrophone->SendAudioData(a_Data);
}

void TelephonyAgent::OnConfirm(const ThingEvent & a_Event)
{
	Confirm::SP spConfirm = DynamicCast<Confirm>(a_Event.GetIThing());
	if (spConfirm && spConfirm->GetConfirmType() == "PhoneCall")
	{
		if (spConfirm->IsConfirmed())
		{
			Log::Status("TelephonyAgent", "Accepting incoming call from %s", m_FromNumber.c_str());
			ITelephony * pTelephony = Config::Instance()->FindService<ITelephony>();
			if (pTelephony != NULL && pTelephony->Answer(m_FromNumber, m_ToNumber))
				BeginCall();
		}
		else
		{
			Log::Status("TelephonyAgent", "Declining incoming call from %s", m_FromNumber.c_str());
			m_spActiveIntent.reset();
		}
	}
}

void TelephonyAgent::OnLearnTelephoneNumber(const ThingEvent & a_Event)
{
	LearningIntent::SP spIntent = DynamicCast<LearningIntent>(a_Event.GetIThing());
	if (spIntent->GetVerb() == "learn_number" && !spIntent->GetPhoneNumber().empty())
	{
		m_ToNumber = spIntent->GetPhoneNumber();

		Say::SP spSay(new Say(m_LearnedPhoneNumberUtterance));
		SelfInstance * pInstance = SelfInstance::GetInstance();
		pInstance->GetBlackBoard()->AddThing(spSay);
	}
}