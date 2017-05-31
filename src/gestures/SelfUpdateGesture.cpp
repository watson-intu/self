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


#include "SelfUpdateGesture.h"
#include "SelfInstance.h"
#include "../agent/UpdateAgent.h"

REG_SERIALIZABLE( SelfUpdateGesture );
RTTI_IMPL( SelfUpdateGesture, IGesture );

void SelfUpdateGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize( json );
}

void SelfUpdateGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize( json );
}

bool SelfUpdateGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	UpdateAgent * pSelfUpdateAgent = SelfInstance::GetInstance()->FindAgent<UpdateAgent>();

	if(pSelfUpdateAgent != NULL)
	{
		pSelfUpdateAgent->OnManualUpgrade();
	}

	return true;
}

bool SelfUpdateGesture::Abort()
{
	return false;
}