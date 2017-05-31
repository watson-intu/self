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


#include "EmailGesture.h"
#include "SelfInstance.h"
#include "services/IMail.h"
#include "utils/Form.h"

REG_SERIALIZABLE(EmailGesture);
RTTI_IMPL( EmailGesture, IGesture );

void EmailGesture::Serialize(Json::Value & json)
{
	json["m_To"] = m_To;
	json["m_Subject"] = m_Subject;
	json["m_Message"] = m_Message;
}

void EmailGesture::Deserialize(const Json::Value & json)
{
	if ( json.isMember("m_To") )
		m_To = json["m_To"].asString();
	if ( json.isMember("m_Subject") )
		m_Subject = json["m_Subject"].asString();
	if ( json.isMember("m_Message") )
		m_Message = json["m_Message"].asString();
}

bool EmailGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
    if ( PushRequest( a_Callback, a_Params ) )
        Send();

    return true;
}

bool EmailGesture::Abort()
{
    return false;
}

void EmailGesture::Send()
{
    Request * pReq = ActiveRequest();
    assert( pReq != NULL );

	IMail * pService = Config::Instance()->FindService<IMail>();
    if ( pService != NULL )
    {
		const Json::Value & data = pReq->m_Params.GetData();
		if ( data.isMember("to") )
			m_To = data["to"].asString();
		if ( data.isMember("subject") )
		    m_Subject = data["subject"].asString();
        if ( data.isMember("message") )
            m_Message = data["message"].asString();

		std::string message = pReq->m_Params.ResolveVariables( m_Message );
		if ( data["attachments"].isArray() )
		{
			std::string boundry( UniqueID().Get() );
			std::stringstream ss;
			ss << "MIME-Version: 1.0\r\n";
			ss << "Content-Type: multipart/mixed; boundary=\"" << boundry << "\"\r\n\r\n";

			ss << "--" << boundry << "\r\n";
			ss << "Content-Type: text/plain\r\n\r\n";
			ss << message;

			const Json::Value & attachments = data["attachments"];
			for(size_t i=0;i<attachments.size();++i)
			{
				const Json::Value & attachment = attachments[i];
				bool bInline = attachment["inline"].asBool();
				std::string filename = attachment["filename"].asString();
				if ( filename.size() == 0 )
				{
					Log::Warning( "EmailGesture", "Filename field for attachment is required." );
					continue;
				}
				std::string type = attachment["type"].asString();
				std::string encoding = attachment["encoding"].asString();

				std::string data = attachment["body"].asString();

				ss << "--" << boundry << "\r\n";
				ss << "Content-Disposition: " << (bInline ? "inline" : "attachment") << "; filename=\"" << filename << "\"\r\n";
				if ( type.size() > 0 )
					ss << "Content-Type: " << type << "\r\n";
				if ( encoding.size() > 0 )
					ss << "Content-Transfer-Encoding: " << encoding << "\r\n";

				ss << "\r\n" << data << "\r\n";
			}

			// ending boundry..
			ss << "--" << boundry << "--\r\n";
			// replace the message
			message = ss.str();
		}

        pService->SendEmail( 
			pReq->m_Params.ResolveVariables( m_To ),
			pReq->m_Params.ResolveVariables( m_Subject ),
			message,
            DELEGATE( EmailGesture, OnSent, IService::Request *, this ) );
    }
    else
	{
		Log::Warning( "EmailGesture", "No IMail service found." );
        OnSent(NULL);
	}
}

void EmailGesture::OnSent( IService::Request * a_pRequest )
{
    Request * pReq = ActiveRequest();
    assert( pReq != NULL );

    if ( a_pRequest == NULL || a_pRequest->IsError() )
	{
        Log::Error( "EmailGesture", "Failed to send Email %s", m_Message.c_str() );
		pReq->m_bError = true;
	}

    if ( PopRequest() )
        Send();
}

