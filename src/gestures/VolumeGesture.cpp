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


#include "VolumeGesture.h"

REG_SERIALIZABLE( VolumeGesture );
RTTI_IMPL( VolumeGesture, IGesture );

void VolumeGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize( json );

	json["m_fChange"] = m_fChange;
	json["m_fTargetVolume"] = m_fTargetVolume;
}

void VolumeGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize( json );

	m_fChange = json["m_fChange"].asFloat();
	m_fTargetVolume = json["m_fTargetVolume"].asFloat();
}

bool VolumeGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	if ( a_Callback.IsValid() )
		a_Callback( this );
	return true;
}

