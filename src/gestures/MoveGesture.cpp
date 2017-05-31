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


#include "MoveGesture.h"

REG_SERIALIZABLE( MoveGesture );
RTTI_IMPL( MoveGesture, IGesture );

void MoveGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize( json );

	json["m_fX"] = m_fX;
	json["m_fY"] = m_fY;
	json["m_fZ"] = m_fZ;
}

void MoveGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize( json );

	m_fX = json["m_fX"].asFloat();
	m_fY = json["m_fY"].asFloat();
	m_fZ = json["m_fZ"].asFloat();
}

bool MoveGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	Log::Status( "MoveGesture", "Execute move gesture, x = %f, y = %f, z = %f", m_fX, m_fY, m_fZ );
	if ( a_Callback.IsValid() )
		a_Callback( this );
	return true;
}

bool MoveGesture::Abort()
{
	return false;
}

