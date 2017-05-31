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


#include "SocketGesture.h"
#include "utils/WebClientService.h"

REG_SERIALIZABLE( SocketGesture );
RTTI_IMPL( SocketGesture, IGesture );

SocketGesture::SocketGesture() : m_pSocket( NULL )
{}

SocketGesture::~SocketGesture()
{
    if (m_pSocket != NULL)
    {
        m_pSocket->close();
        delete m_pSocket;
    }
}

void SocketGesture::Serialize(Json::Value & json)
{
    IGesture::Serialize(json);

    json["m_Ip"] = m_Ip;
    json["m_Port"] = m_Port;
    json["m_Message"] = m_Message;
}

void SocketGesture::Deserialize(const Json::Value & json)
{
    IGesture::Deserialize(json);

    if (json.isMember("m_Ip"))
        m_Ip = json["m_Ip"].asString();
    if (json.isMember("m_Port"))
        m_Port = json["m_Port"].asString();
    if (json.isMember("m_Message"))
        m_Message = json["m_Message"].asString();
}

bool SocketGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
    if ( PushRequest( a_Callback, a_Params ) )
        Send();

    return true;
}

bool SocketGesture::Abort()
{
    return false;
}

void SocketGesture::OnGestureFinished(bool a_bError)
{
    if (!a_bError)
        Log::Debug("SocketGesture", "Gesture finished successfully");
    else
        Log::Error("SocketGesture", "Failed to execute Gesture!");

    if (PopRequest(a_bError))
        Send();
}

void SocketGesture::Send()
{
    try
    { 
		Request * pReq = ActiveRequest();
		assert(pReq != NULL);

		const Json::Value & data = pReq->m_Params.GetData();
		if (data["m_Ip"].isString())
			m_Ip = data["m_Ip"].asString();
		if (data["m_Port"].isString())
			m_Port = data["m_Port"].asString();
		if (data["m_Message"].isString())
			m_Message = data["m_Message"].asString();

        // open the socket connection if needed..
        if ( m_pSocket == NULL )
            m_pSocket = new boost::asio::ip::tcp::socket( WebClientService::Instance()->GetService() );

        if ( !m_pSocket->is_open() )
        {
            boost::asio::ip::tcp::resolver resolver(WebClientService::Instance()->GetService());
            boost::asio::ip::tcp::resolver::query q(m_Ip,  m_Port);
            boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(q);

            boost::asio::connect( *m_pSocket, i );
        }

        boost::asio::write( *m_pSocket, boost::asio::buffer( m_Message ) );
		if (m_pSocket != NULL)
			m_pSocket->close();
		OnGestureFinished(false);
    }
    catch( const std::exception & e )
    {
        Log::Error( "SocketGesture", "Caught Exception: %s", e.what() );
        if ( m_pSocket != NULL )
            m_pSocket->close();
    }
}
