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


#include "Gesture.h"

REG_SERIALIZABLE( Gesture );
RTTI_IMPL( Gesture, IThing );

void Gesture::Serialize(Json::Value & json)
{
    IThing::Serialize( json );
    json["m_Type"] = m_Type;
	json["m_Params"] = ISerializable::SerializeObject(&m_Params);
}

void Gesture::Deserialize(const Json::Value & json)
{
    IThing::Deserialize( json );

    if ( json.isMember("m_Type") )
        m_Type = json["m_Type"].asString();
	if (json.isMember("m_Params"))
		ISerializable::DeserializeObject(json["m_Params"], &m_Params);
}