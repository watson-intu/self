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


#include "AnimateGesture.h"

REG_SERIALIZABLE( AnimateGesture );
RTTI_IMPL( AnimateGesture, IGesture );

void AnimateGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize( json );

	for(size_t i=0;i<m_Animations.size();++i)
	{
		Json::Value itemValue;
		itemValue["m_AnimId"] = m_Animations[i].m_AnimId;
		itemValue["m_RequiredPosture"] = m_Animations[i].m_RequiredPosture;

		json["m_Animations"].append(itemValue);
	}
}

void AnimateGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize( json );

	const Json::Value & animations = json["m_Animations"];
	for( Json::ValueConstIterator iAnim = animations.begin(); iAnim != animations.end(); ++iAnim )
	{
		const Json::Value & item = *iAnim;
		m_Animations.push_back( AnimationEntry( item["m_AnimId"].asString(), item["m_RequiredPosture"].asString() ) );
	}

}

bool AnimateGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	Log::Status( "AnimateGesture", "Execute animate gesture %s.", m_GestureId.c_str() );
	if ( a_Callback.IsValid() )
		a_Callback( this );
	return true;
}

bool AnimateGesture::Abort()
{
	return false;
}

