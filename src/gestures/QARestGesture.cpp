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


#include "QARestGesture.h"
#include "utils/ThreadPool.h"
#include "utils/StringUtil.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"
#include "blackboard/Gesture.h"

REG_SERIALIZABLE( QARestGesture );

RTTI_IMPL( QARestGesture, IGesture );

void QARestGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize( json );

	json["m_URL"] = m_URL;
	json["m_Type"] = m_Type;
	json["m_Body"] = m_Body;
	json["m_UserName"] = m_UserName;
	json["m_Password"] = m_Password;

	int index = 0;
	for( Headers::iterator iHeader = m_Headers.begin(); iHeader != m_Headers.end(); ++iHeader )
	{
		json["m_Headers"][index]["key"] = iHeader->first;
		json["m_Headers"][index++]["value"] = iHeader->second;
	}
}

void QARestGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize( json );

	if ( json.isMember( "m_URL" ) )
		m_URL = json["m_URL"].asString();
	if ( json.isMember( "m_Type" ) )
		m_Type = json["m_Type"].asString();
	if ( json.isMember( "m_Body" ) )
		m_Body = json["m_Body"].asString();

	m_Headers.clear();
	m_Headers["Connection"] = "close";
	if ( json.isMember( "m_Headers" ) )
	{
		const Json::Value & headers = json["m_Headers"];
		for( Json::ValueConstIterator iHeader = headers.begin(); iHeader != headers.end(); ++iHeader )
			m_Headers[ (*iHeader)["key"].asString() ] = (*iHeader)["value"].asString();

		Log::Debug("RestGesture", "after loop headers");
	}

	if(json.isMember("m_UserName") && json.isMember("m_Password") 
		&& json["m_UserName"].asString().length() > 0 && json["m_Password"].asString().length() > 0) {
		m_UserName = json["m_UserName"].asString();
		m_Password = json["m_Password"].asString();

		std::string encodedAuth = StringUtil::EncodeBase64( m_UserName + ":" + m_Password );
		m_Headers["Authorization"] = StringUtil::Format( "Basic %s", encodedAuth.c_str() );
	}
}

bool QARestGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	if ( PushRequest( a_Callback, a_Params ) )
		SendRequest();

	return true;
}

bool QARestGesture::Abort()
{
	return false;
}

void QARestGesture::SendRequest()
{
	Request * pReq = ActiveRequest();
	assert( pReq != NULL );

	const Json::Value & data = pReq->m_Params.GetData();

	if ( data["url"].isString() )
		m_URL = data["url"].asString();
	if ( data["type"].isString() )
		m_Type = data["type"].asString();
	if ( data["body"].isString() )
		m_Body = data["body"].asString();

	
	m_Response.clear();

	std::string m_resolvedURL = pReq->m_Params.ResolveVariables(m_URL);
	std::string m_resolvedType = pReq->m_Params.ResolveVariables(m_Type);
	std::string m_resolvedBody = pReq->m_Params.ResolveVariables(m_Body);

	Log::Debug("QARestGesture", "Sending Request, URL: %s, Type: %s, Body: %s",
		m_resolvedURL.c_str(), m_resolvedType.c_str(), m_resolvedBody.c_str());


	m_spClient = IWebClient::Request( 
		m_resolvedURL,
		m_Headers,									// TODO support ResolveVariables on headers
		m_resolvedType, 
		m_resolvedBody, 
		DELEGATE( QARestGesture, OnData, IWebClient::RequestData *, this ),
		DELEGATE( QARestGesture, OnState, IWebClient *, this ) );
}

void QARestGesture::OnData( IWebClient::RequestData * a_pData )
{
	Log::Debug( "QARestGesture", "OnData() : %u bytes", a_pData->m_Content.size() );

	m_Response += a_pData->m_Content;
	if ( a_pData->m_bDone )
	{
		bool bError = false;
		if ( a_pData->m_StatusCode < 200 || a_pData->m_StatusCode >= 300 )
		{
			Log::Error( "QARestGesture", "QARestGesture failed: %s", m_Response.c_str() );
			bError = true;
		}
		else
		{
			Log::Debug("RestGesture", "RestGesture completed: %s", m_Response.c_str());
			Json::Value myJson;
			Json::Value parsedResponse;
			Json::Reader reader;
			Json::FastWriter fastWriter;
			bool parsingSuccessful = reader.parse(m_Response.c_str(), parsedResponse);     //parse process
			if (parsingSuccessful)
			{
				if ( parsedResponse.isMember( "speech" ) && parsedResponse.isMember( "visual" ) )
				{
					ParamsMap params;
					params["visual"] = fastWriter.write(parsedResponse["visual"]);
					SelfInstance::GetInstance()->GetBlackBoard()->AddThing(Gesture::SP(new Gesture("send_qa_response", params)));
					SelfInstance::GetInstance()->GetBlackBoard()->AddThing(Say::SP(new Say(parsedResponse["speech"].asString())));
				}
				else if ( parsedResponse.isMember( "speech" ) )
				{
					SelfInstance::GetInstance()->GetBlackBoard()->AddThing(Say::SP(new Say(std::string(parsedResponse["speech"].asString()))));
				}
				else if ( parsedResponse.isMember( "visual" ) )
				{
					Log::Debug( "RestGesture", "RestGesture got screen: %s", parsedResponse["visual"].asString().c_str() );
					ParamsMap params;
					params["visual"] = fastWriter.write(parsedResponse["visual"]);
					SelfInstance::GetInstance()->GetBlackBoard()->AddThing(Gesture::SP(new Gesture("send_qa_response", params)));
				}
					
			}
		}

		if ( PopRequest( bError ) )
			SendRequest();
	}
}

void QARestGesture::OnState( IWebClient * a_pClient )
{
	Log::Debug( "QARestGesture", "OnState() : %u", a_pClient->GetState() );
}