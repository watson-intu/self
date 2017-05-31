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


#ifndef QA_REST_GESTURE_H
#define QA_REST_GESTURE_H

// forward declare 
namespace happyhttp { class Response; }

#include "IGesture.h"
#include "utils/IWebClient.h"
#include "SelfLib.h"

//! This gesture is for making a REST call to a server.
class SELF_API QARestGesture : public IGesture
{
public:
	RTTI_DECL();

	//! Types
	typedef IWebClient::Headers		Headers;

	//! Construction
	QARestGesture() : m_Type( "GET" )
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IGesture interface
	virtual bool Execute( GestureDelegate a_Callback, const ParamsMap & a_Params );
	virtual bool Abort();

	void SetURL( const std::string & a_URL )
	{
		m_URL = a_URL;
	}
	void SetRequestType( const std::string & a_Type )
	{
		m_Type = a_Type;
	}
	void SetHeaders( const Headers & a_Headers )
	{
		m_Headers = a_Headers;
	}
	void SetBody( const std::string & a_Body )
	{
		m_Body = a_Body;
	}
	void SetUserName( const std::string & a_UserName )
	{
		m_UserName = a_UserName;
	}
	void SetPassword( const std::string & a_Password )
	{
		m_Password = a_Password;
	}
	
protected:
	//! Data
	IWebClient::SP		m_spClient;
	std::string			m_URL;			// url of the rest request
	std::string			m_Type;			// GET, POST, or DELETE
	Headers				m_Headers;		// headers for the request
	std::string			m_Body;			// body of the request
	std::string			m_UserName;
	std::string			m_Password;
	std::string			m_Response;
	bool				m_bInternalRequest;

	struct Context 
	{
		Context() : m_Error( false )
		{}

		GestureDelegate		m_Callback;
		ParamsMap			m_Params;
		std::string			m_Response;
		bool				m_Error;

	};

	void SendRequest();
	void OnData( IWebClient::RequestData * a_pData );
	void OnState( IWebClient * a_pClient );
};


#endif //IGESTURE_H
