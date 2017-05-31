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


#include "GraspGesture.h"

REG_SERIALIZABLE( GraspGesture );
RTTI_IMPL( GraspGesture, IGesture );

void GraspGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize( json );

	json["m_Hand"] = m_Hand;
	json["m_Open"] = m_Open;
}

void GraspGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize( json );

	m_Hand = json["m_Hand"].asString();
	m_Open = json["m_Open"].asBool();
}

bool GraspGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	Log::Debug( "GraspGesture", "Execute grasp gesture" );
	if ( a_Callback.IsValid() )
		a_Callback( this );
	return true;
}

bool GraspGesture::Abort()
{
	return false;
}

