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


#ifndef SELF_WEBREQUESTAGENT_H
#define SELF_WEBREQUESTAGENT_H

#include "IAgent.h"
#include "utils/IWebClient.h"
#include "blackboard/WebRequest.h"

class SkillInstance;

class SELF_API WebRequestAgent : public IAgent
{
public:
    RTTI_DECL();

    WebRequestAgent() 
    {}

	~WebRequestAgent();

    //! ISerializable interface
    void Serialize( Json::Value & json );
    void Deserialize( const Json::Value & json );

    //! IAgent interface
    virtual bool OnStart();
    virtual bool OnStop();

private:

    //! Types
    typedef std::list<WebRequest::SP>		RequestList;

    //! Data
    WebRequest::SP  m_spActive;
    RequestList     m_Requests;
    IWebClient::SP  m_spClient;

    //! Event Handlers
    void OnWebRequest(const ThingEvent & a_ThingEvent);
    void ExecuteRequest(WebRequest::SP a_spWebRequest);
    void OnResponse(IWebClient::RequestData * a_pResponse);
    void OnState(IWebClient * a_pConnector);
};

#endif //SELF_WEBREQUESTAGENT_H
