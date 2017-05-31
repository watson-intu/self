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


#include "SystemGesture.h"
#include "utils/ThreadPool.h"

REG_SERIALIZABLE( SystemGesture );
RTTI_IMPL( SystemGesture, IGesture );

void SystemGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize( json );
	json["m_SystemId"] = m_SystemId;
}

void SystemGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize( json );
	if ( json.isMember( "m_SystemId" ) )
		m_SystemId = json["m_SystemId"].asString();
}

bool SystemGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	if ( m_SystemId == "reboot" )
		ThreadPool::Instance()->StopMainThread(1);
	else if ( m_SystemId == "shutdown" )
		ThreadPool::Instance()->StopMainThread(0);
	else
		Log::Warning( "SystemGesture", "Unsupported system gesture %s", m_SystemId.c_str() );

	if ( a_Callback.IsValid() )
		a_Callback( this );
	return true;
}

