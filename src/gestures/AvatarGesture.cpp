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


#include "AvatarGesture.h"
#include "services/IAvatar.h"
#include "utils/TimerPool.h"
#include "SelfInstance.h"

REG_SERIALIZABLE(AvatarGesture);
RTTI_IMPL(AvatarGesture, IGesture);

void AvatarGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize(json);
	json["m_StateId"] = m_StateId;
}

void AvatarGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize(json);
	if ( json.isMember( "m_StateId" ) )
		m_StateId = json["m_StateId"].asString();
}

bool AvatarGesture::Execute(GestureDelegate a_Callback, const ParamsMap & a_Params)
{
	IAvatar * pAvatarService = SelfInstance::GetInstance()->FindService<IAvatar>();
	if ( pAvatarService != NULL)
	{
		if (! pAvatarService->ChangeState( m_StateId ) )
			Log::Error( "WatsonAvatarGesture", "Failed to notify avatar" );
	}

	if ( a_Callback.IsValid() )
		a_Callback( this );

	return true;
}

bool AvatarGesture::Abort()
{
	return false;
}

