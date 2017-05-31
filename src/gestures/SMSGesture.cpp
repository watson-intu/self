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


#include "SMSGesture.h"
#include "SelfInstance.h"
#include "services/ITelephony.h"

REG_SERIALIZABLE(SMSGesture);
RTTI_IMPL( SMSGesture, IGesture );

void SMSGesture::Serialize(Json::Value & json)
{
	json["m_Number"] = m_Number;
	json["m_Message"] = m_Message;
}

void SMSGesture::Deserialize(const Json::Value & json)
{
	if ( json.isMember("m_Number") )
		m_Number = json["m_Number"].asString();
	if ( json.isMember("m_Message") )
		m_Message = json["m_Message"].asString();
}

bool SMSGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	if ( PushRequest( a_Callback, a_Params ) )
		Send();

	return true;
}

bool SMSGesture::Abort()
{
	return false;
}

void SMSGesture::Send()
{
	Request * pReq = ActiveRequest();
	assert( pReq != NULL );

	ITelephony * pService = SelfInstance::GetInstance()->FindService<ITelephony>();
	if ( pService != NULL )
	{
		if ( pReq->m_Params.GetData().isMember("number") )
			m_Number = pReq->m_Params["number"].asString();
		if ( pReq->m_Params.GetData().isMember("message") )
			m_Message = pReq->m_Params["message"].asString();

		OnSent( pService->Text( 
			pReq->m_Params.ResolveVariables( m_Number ),
			pReq->m_Params.ResolveVariables( m_Message ) ) == false );
	}
	else
		OnSent( true );
}

void SMSGesture::OnSent( bool a_bError )
{
	if ( PopRequest( a_bError ) )
		Send();
}