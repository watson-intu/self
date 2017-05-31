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


#include "RestGesture.h"
#include "utils/ThreadPool.h"
#include "utils/StringUtil.h"

REG_SERIALIZABLE( RestGesture );
RTTI_IMPL( RestGesture, IGesture );

void RestGesture::Serialize(Json::Value & json)
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

void RestGesture::Deserialize(const Json::Value & json)
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

bool RestGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	if ( PushRequest( a_Callback, a_Params ) )
		SendRequest();

	return true;
}

bool RestGesture::Abort()
{
	return false;
}

void RestGesture::SendRequest()
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

	Log::Debug( "RestGesture", "Sending Request, URL: %s, Type: %s, Body: %s", 
		m_URL.c_str(), m_Type.c_str(), m_Body.c_str() );

	m_Response.clear();

	m_spClient = IWebClient::Request( 
		pReq->m_Params.ResolveVariables( m_URL ),
		m_Headers,									// TODO support ResolveVariables on headers
		pReq->m_Params.ResolveVariables( m_Type ), 
		pReq->m_Params.ResolveVariables( m_Body ), 
		DELEGATE( RestGesture, OnData, IWebClient::RequestData *, this ),
		DELEGATE( RestGesture, OnState, IWebClient *, this ) );
}

void RestGesture::OnData( IWebClient::RequestData * a_pData )
{
	Log::Debug( "RestGesture", "OnData() : %u bytes", a_pData->m_Content.size() );

	m_Response += a_pData->m_Content;
	if ( a_pData->m_bDone )
	{
		bool bError = false;
		if ( a_pData->m_StatusCode < 200 || a_pData->m_StatusCode >= 300 )
		{
			Log::Error( "RestGesture", "RestGesture failed: %s", m_Response.c_str() );
			bError = true;
		}
		else
			Log::Debug( "RestGesture", "RestGesture completed: %s", m_Response.c_str() );

		if ( PopRequest( bError ) )
			SendRequest();
	}
}

void RestGesture::OnState( IWebClient * a_pClient )
{
	Log::Debug( "RestGesture", "OnState() : %u", a_pClient->GetState() );
}


