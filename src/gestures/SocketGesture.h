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


#ifndef SELF_SOCKETGESTURE_H
#define SELF_SOCKETGESTURE_H


#include "boost/asio.hpp"
#include "IGesture.h"
#include "SelfLib.h"

class SELF_API SocketGesture : public IGesture
{
public:
    RTTI_DECL();

    //! Construction
    SocketGesture();
    ~SocketGesture();

    //! ISerializable interface
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

    //! IGesture interface
    virtual bool Execute(GestureDelegate a_Callback, const ParamsMap & a_Params);
    virtual bool Abort();

    //! Mutators
    void SetIp(const std::string & a_Ip)
    {
        m_Ip = a_Ip;
    }
    void SetPort(const std::string & a_Port)
    {
        m_Port = a_Port;
    }
    void SetMessage(const std::string & a_Message)
    {
        m_Message = a_Message;
    }

protected:
    void Send();
    void OnGestureFinished(bool a_Response);

    //! Data
    std::string			m_Ip;
    std::string			m_Port;
    std::string         m_Message;

    boost::asio::ip::tcp::socket *m_pSocket;
};

#endif //SELF_SOCKETGESTURE_H
