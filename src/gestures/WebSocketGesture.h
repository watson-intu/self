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


#ifndef SELF_WEB_SOCKET_GESTURE_H
#define SELF_WEB_SOCKET_GESTURE_H

#include "utils/IService.h"
#include "IGesture.h"
#include "SelfLib.h"

//! This gesture can send messages over a web socket.
class SELF_API WebSocketGesture : public IGesture
{
public:
	RTTI_DECL();

	//! Types
	typedef IWebClient::Headers		Headers;

	//! Construction
	WebSocketGesture()
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IGesture interface
	virtual bool Execute(GestureDelegate a_Callback, const ParamsMap & a_Params);
	virtual bool Abort();

	//! Mutators
	void SetURL(const std::string & a_URL)
	{
		m_URL = a_URL;
	}
	void SetHeaders(const Headers & a_Headers)
	{
		m_Headers = a_Headers;
	}
	void SetMessage(const std::string & a_Message)
	{
		m_Message = a_Message;
	}
	void SetUserName(const std::string & a_UserName)
	{
		m_UserName = a_UserName;
	}
	void SetPassword(const std::string & a_Password)
	{
		m_Password = a_Password;
	}

protected:
	void Send();
	void OnGestureFinished(bool a_Response);

	//! Data
	std::string			m_URL;
	std::string			m_Message;
	Headers				m_Headers;
	std::string			m_UserName;
	std::string			m_Password;

private:
	struct Connection : public boost::enable_shared_from_this<Connection>
	{
		//! Types
		typedef boost::shared_ptr<Connection>		SP;
		typedef boost::weak_ptr<Connection>			WP;
		typedef std::list<IWebSocket::FrameSP>	FramesList;

		Connection(WebSocketGesture * a_pGesture, Delegate<bool> a_Callback);
		~Connection();

		WebSocketGesture *	m_pGesture;
		IWebClient::SP		m_spSocket;          // use to communicate with the server
		bool				m_Connected;
		Delegate<bool>		m_Callback;

		bool Start();
		void SendText();
		bool CreateConnector();
		void CloseConnector();

		void OnListenMessage(IWebSocket::FrameSP a_spFrame);
		void OnListenState(IWebClient *);
		void OnListenData(IWebClient::RequestData *);

	};

	//! Types
	typedef std::list<Connection::SP>		Connectionlist;

	//! Data
	Connectionlist	m_Connections;
};


#endif // SELF_WEB_SOCKET_GESTURE_H
