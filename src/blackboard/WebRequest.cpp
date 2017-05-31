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


#include "WebRequest.h"

REG_SERIALIZABLE( WebRequest );
RTTI_IMPL( WebRequest, IThing );

void WebRequest::Serialize(Json::Value & json)
{
    IThing::Serialize( json );

    json["m_URL"] = m_URL;
    json["m_Type"] = m_Type;
    json["m_Body"] = m_Body;
    json["m_Params"] = m_Params;

    int index = 0;
    for( Headers::iterator iHeader = m_Headers.begin(); iHeader != m_Headers.end(); ++iHeader )
    {
        json["m_Headers"][index]["key"] = iHeader->first;
        json["m_Headers"][index++]["value"] = iHeader->second;
    }
}

void WebRequest::Deserialize(const Json::Value & json)
{
    IThing::Deserialize( json );

    // Required Fields
    m_URL = json["m_URL"].asString();
    m_Type = json["m_Type"].asString();

    // Optional Fields
    if( json.isMember( "m_Body" ) )
        m_Body = json["m_Body"].asString();
    if( json.isMember( "m_Params" ) )
        m_Params = json["m_Params"].asString();
    m_Headers.clear();
    if ( json.isMember( "m_Headers" ) )
    {
        const Json::Value & headers = json["m_Headers"];
        for( Json::ValueConstIterator iHeader = headers.begin(); iHeader != headers.end(); ++iHeader )
            m_Headers[ (*iHeader)["key"].asString() ] = (*iHeader)["value"].asString();
    }
}