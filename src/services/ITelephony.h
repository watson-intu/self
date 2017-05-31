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


#ifndef SELF_ITELEPHONY_H
#define SELF_ITELEPHONY_H

#include "utils/IService.h"
#include "SelfLib.h"			// include last always

//! This service interfaces with the telephony gateway to allow for sending & accepting phone calls and SMS messages.
class SELF_API ITelephony : public IService
{
public:
	RTTI_DECL();

	//! Types
	typedef Delegate<const Json::Value &>		OnCommand;
	typedef Delegate<const std::string &>		OnAudioOut;

	//! Construction
	ITelephony(const std::string & a_ServiceId, AuthType a_AuthType = AUTH_BASIC ) : IService( a_ServiceId, a_AuthType )
	{}

	//! ITelephony interface
	virtual const std::string &	GetAudioInFormat() const = 0;
	virtual const std::string &	GetAudioOutFormat() const = 0;
	virtual const std::string & GetMyNumber() const = 0;

	//! Connect to the back-end, this will register and we will become available to receive phone calls. The provided
	//! Callback will be invoke when a call is incoming, the user must call Answer() to answer the incoming call.
	virtual bool Connect( const std::string & a_SelfId,
		OnCommand a_OnCommand, 
		OnAudioOut a_OnAudioOut ) = 0;
	//! Make outgoing call.
	virtual bool Dial( const std::string & a_Number ) = 0;
	//! Answer an incoming call, returns true on success.
	virtual bool Answer( const std::string & fromNumber,
		const std::string & toNumber ) = 0;
	//! Hang up current call.
	virtual bool HangUp() = 0;
	//! Send a SMS message
	virtual bool Text( const std::string & a_Number, 
		const std::string & a_Message ) = 0;
	//! Disconnect from the back-end..
	virtual bool Disconnect() = 0;
	//! Send binary audio data up to the gateway, the format of the audio must match the format 
	//! specified by GetAudioFormat(), usually audio/L16;rate=16000
	virtual void SendAudioIn( const std::string & a_Audio ) = 0;
};

#endif 