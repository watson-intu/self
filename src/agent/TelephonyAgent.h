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


#ifndef SELF_TELEPHONY_AGENT_H
#define SELF_TELEPHONY_AGENT_H

#include "IAgent.h"
#include "blackboard/Confirm.h"
#include "blackboard/TelephonyIntent.h"
#include "sensors/TelephonyMicrophone.h"
#include "gestures/TelephonySpeechGesture.h"
#include "services/ITelephony.h"
#include "SelfLib.h"

//! This agent handles making and answering phone calls
class SELF_API TelephonyAgent : public IAgent
{
public:
	RTTI_DECL();

	TelephonyAgent();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Data
	std::string                 m_TelephonyUtterance;
	std::string                 m_LearnedPhoneNumberUtterance;
	std::string                 m_FromNumber;
	std::string                 m_ToNumber;

	TelephonyIntent::SP         m_spActiveIntent;
	unsigned int                m_AudioSent;
	unsigned int                m_AudioRecv;
	bool						m_bInCall;

	TelephonyMicrophone::SP     m_pTelephonyMicrophone;
	TelephonySpeechGesture::SP  m_pTelephonySpeechGesture;

	void                        BeginCall();
	void                        EndCall();

	//! Event Handlers
	void                        OnTelephonyIntent(const ThingEvent & a_ThingEvent);
	void                        ExecuteRequest( TelephonyIntent::SP a_pTelephony );
	void                        OnCommand( const Json::Value & a_Call );
	void                        OnAudioOut( const std::string & a_Data );
	void                        OnConfirm( const ThingEvent & a_Event );
	void                        OnLearnTelephoneNumber( const ThingEvent & a_Event );

};

#endif // SELF_TELEPHONY_AGENT_H