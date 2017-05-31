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


#include "MoveJointGesture.h"

REG_SERIALIZABLE( MoveJointGesture );
RTTI_IMPL( MoveJointGesture, IGesture );

void MoveJointGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize( json );

	json["m_fStiffness"] = m_fStiffness;
	json["m_bAbsolute"] = m_bAbsolute;

	SerializeVector("m_JointNames", m_JointNames, json);
	SerializeVector("m_fAngles", m_fAngles, json);
	SerializeVector("m_fSpeeds", m_fSpeeds, json);
}

void MoveJointGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize( json );

	m_fStiffness = json["m_fStiffness"].asFloat();
	m_bAbsolute = json["m_bAbsolute"].asBool();

	DeserializeVector("m_JointNames", json, m_JointNames);
	DeserializeVector("m_fAngles", json, m_fAngles);
	DeserializeVector("m_fSpeeds", json, m_fSpeeds);

	if(m_JointNames.size() == 0)
		m_JointNames.push_back(json["m_JointName"].asString());
	if(m_fAngles.size() == 0)
		m_fAngles.push_back(json["m_fAngle"].asFloat());
	if(m_fSpeeds.size() == 0)
		m_fSpeeds.push_back(json["m_fSpeed"].asFloat());
}

bool MoveJointGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	for(size_t i=0;i<m_JointNames.size();++i)
	{
		Log::Status( "MoveJointGesture", "Moving joint %s to angle: %f, speed: %f, stiffness: %f, absolute: %s",
			m_JointNames[i].c_str(), m_fAngles[i], m_fSpeeds[i], m_fStiffness, m_bAbsolute ? "Yes" : "No" );
	}

	if ( a_Callback.IsValid() )
		a_Callback( this );
	return true;
}

