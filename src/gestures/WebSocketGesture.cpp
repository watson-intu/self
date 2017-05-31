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


#include "WebSocketGesture.h"

REG_SERIALIZABLE(WebSocketGesture);
RTTI_IMPL(WebSocketGesture, IGesture);

void WebSocketGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize(json);

	json["m_URL"] = m_URL;
	json["m_Message"] = m_Message;
	json["m_UserName"] = m_UserName;
	json["m_Password"] = m_Password;

	int index = 0;
	for (Headers::iterator iHeader = m_Headers.begin(); iHeader != m_Headers.end(); ++iHeader)
	{
		json["m_Headers"][index]["key"] = iHeader->first;
		json["m_Headers"][index++]["value"] = iHeader->second;
	}
}

void WebSocketGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize(json);

	if (json.isMember("m_URL"))
		m_URL = json["m_URL"].asString();
	if (json.isMember("m_Message"))
		m_Message = json["m_Message"].asString();

	m_Headers.clear();

	if (json.isMember("m_Headers"))
	{
		const Json::Value & headers = json["m_Headers"];
		for (Json::ValueConstIterator iHeader = headers.begin(); iHeader != headers.end(); ++iHeader)
			m_Headers[(*iHeader)["key"].asString()] = (*iHeader)["value"].asString();
	}

	if (json.isMember("m_UserName") && json.isMember("m_Password")
		&& json["m_UserName"].asString().length() > 0 && json["m_Password"].asString().length() > 0) 
	{
		m_UserName = json["m_UserName"].asString();
		m_Password = json["m_Password"].asString();

		std::string encodedAuth = StringUtil::EncodeBase64(m_UserName + ":" + m_Password);
		m_Headers["Authorization"] = StringUtil::Format("Basic %s", encodedAuth.c_str());
	}
}

bool WebSocketGesture::Execute(GestureDelegate a_Callback, const ParamsMap & a_Params)
{
	if (PushRequest(a_Callback, a_Params))
		Send();

	return true;
}

bool WebSocketGesture::Abort()
{
	return false;
}

void WebSocketGesture::Send()
{
	Request * pReq = ActiveRequest();
	assert(pReq != NULL);

	Connection::SP spConnection( new Connection( this, DELEGATE( WebSocketGesture, OnGestureFinished, bool, this ) ) );
	if (!spConnection->Start())
		Log::Error("WebSocketGesture", "Failed to start WebSocketGesture");
	else
		m_Connections.push_back(spConnection);
}

void WebSocketGesture::OnGestureFinished(bool a_bError)
{
	if (!a_bError)
		Log::Debug("WebSocketGesture", "Gesture finished successfully");
	else
		Log::Error("WebSocketGesture", "Failed to execute Gesture!");

	if (PopRequest(a_bError))
		Send();
}

// ----------------------------

WebSocketGesture::Connection::Connection(WebSocketGesture * a_pGesture, Delegate<bool> a_Callback) :
	m_pGesture(a_pGesture),
	m_Callback(a_Callback),
	m_Connected(false)
{}

WebSocketGesture::Connection::~Connection()
{
	Log::Debug("WebSocketGesture", "Deleting Connection Object");
	CloseConnector();
}

bool WebSocketGesture::Connection::Start()
{
	if (!CreateConnector())
	{
		m_Callback(true);
		return false;
	}
	else
	{
		SendText();
		return true;
	}
}

bool WebSocketGesture::Connection::CreateConnector()
{
	if (!m_spSocket)
	{
		std::string url = m_pGesture->m_URL;
		const Json::Value & data = m_pGesture->ActiveRequest()->m_Params.GetData();
		if ( data["url"].isString() )
			url = data["url"].asString();

		StringUtil::Replace(url, "https://", "wss://", true);
		StringUtil::Replace(url, "http://", "ws://", true);

		m_spSocket = IWebClient::Create( m_pGesture->ActiveRequest()->m_Params.ResolveVariables(url) );
		m_spSocket->SetHeaders(m_pGesture->m_Headers);
		m_spSocket->SetFrameReceiver(DELEGATE(Connection, OnListenMessage, IWebSocket::FrameSP, shared_from_this()));
		m_spSocket->SetStateReceiver(DELEGATE(Connection, OnListenState, IWebClient *, shared_from_this()));
		m_spSocket->SetDataReceiver(DELEGATE(Connection, OnListenData, IWebClient::RequestData *, shared_from_this()));

		if (!m_spSocket->Send())
		{
			m_Connected = false;
			Log::Error("WebSocketGesture", "Failed to connect web socket to %s", url.c_str());
			return false;
		}
	}

	return true;
}

void WebSocketGesture::Connection::CloseConnector()
{
	m_spSocket->SendClose("");
	m_spSocket.reset();
	m_Callback(false);
}

void WebSocketGesture::Connection::SendText()
{
	if (!m_spSocket)
	{
		throw WatsonException("SendText() called with null connector.");
	}

	std::string message = m_pGesture->m_Message;
	const Json::Value & data = m_pGesture->ActiveRequest()->m_Params.GetData();
	if (data["message"].isString())
		message = data["message"].asString();
	Log::Debug("WebSocketGesture", "Sending the following payload: %s", message.c_str());
	m_spSocket->SendText(m_pGesture->ActiveRequest()->m_Params.ResolveVariables(message));
	m_pGesture->m_Connections.remove(shared_from_this());
}

void WebSocketGesture::Connection::OnListenMessage(IWebSocket::FrameSP a_spFrame)
{
	if (a_spFrame->m_Op == IWebSocket::TEXT_FRAME)
	{
		Json::Value json;
		Json::Reader reader(Json::Features::strictMode());
		if (reader.parse(a_spFrame->m_Data, json))
		{
			Log::Debug("WebSocketGesture", "Received text message from server: %s", json.toStyledString().c_str());
		}
		else
			Log::Error("WebSocketGesture", "Failed to parse response from server: %s", a_spFrame->m_Data.c_str());
	}
}

void WebSocketGesture::Connection::OnListenState(IWebClient * a_pClient)
{
	if (a_pClient == m_spSocket.get())
	{
		if (a_pClient->GetState() == IWebClient::DISCONNECTED || a_pClient->GetState() == IWebClient::CLOSED)
		{
			m_Callback(false);
		}
		else if (a_pClient->GetState() == IWebClient::CONNECTED)
		{
			m_Connected = true;
		}
	}
}

void WebSocketGesture::Connection::OnListenData(IWebClient::RequestData * a_pData)
{}