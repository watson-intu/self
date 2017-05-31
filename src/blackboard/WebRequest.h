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


#ifndef SELF_WEBREQUEST_H
#define SELF_WEBREQUEST_H

#include "IThing.h"
#include "utils/IWebClient.h"

//! This object is placed on the blackboard when we need self to make a rest request
class SELF_API WebRequest : public IThing
{
public:
    RTTI_DECL();

    //! Types
    typedef boost::shared_ptr<WebRequest>			SP;
    typedef boost::weak_ptr<WebRequest>			WP;
    typedef IWebClient::Headers		Headers;

    //! ISerializable interface
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

    //! Construction
    WebRequest() : IThing( TT_COGNITIVE )
    {}

    WebRequest( const std::string & a_URL,
                const std::string & a_Type,
                const std::string & a_Body,
                const std::string & a_Params,
                const Headers & a_Headers) :
            IThing( TT_COGNITIVE ),
            m_URL ( a_URL ),
            m_Type ( a_Type ),
            m_Body ( a_Body ),
            m_Params ( a_Params ),
			m_Headers ( a_Headers )
	{}

    //! Accessors
    const std::string & GetParams() const
    {
        return m_Params;
    }

    void SetParams( const std::string & a_Params )
    {
        m_Params = a_Params;
    }

    const std::string & GetURL() const
    {
        return m_URL;
    }

    void SetURL( const std::string & a_URL )
    {
        m_URL = a_URL;
    }

    const std::string & GetType() const
    {
        return m_Type;
    }

    void SetType( const std::string & a_Type )
    {
        m_Type = a_Type;
    }

    const std::string & GetBody() const
    {
        return m_Body;
    }

    void SetBody( const std::string & a_Body )
    {
        m_Body = a_Body;
    }

    const Headers & GetHeaders() const
    {
        return m_Headers;
    }

    void SetHeaders( const Headers & a_Headers)
    {
        m_Headers = a_Headers;
    }

private:

    //! Data
    std::string         m_Params;
    std::string         m_URL;
    std::string         m_Type;
    std::string         m_Body;
    Headers             m_Headers;
};

#endif //SELF_WEBREQUEST_H
