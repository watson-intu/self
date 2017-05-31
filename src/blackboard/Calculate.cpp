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


#include "Calculate.h"

REG_SERIALIZABLE(Calculate);
RTTI_IMPL(Calculate, IThing);

void Calculate::Serialize(Json::Value & json)
{
	IThing::Serialize(json);

	json["m_Data"] = m_Data;
	json["m_Arithmetic"] = m_Arithmetic;
	json["m_Key"] = m_Key;
}

void Calculate::Deserialize(const Json::Value & json)
{
	IThing::Deserialize(json);

	if (json.isMember("m_Data"))
		m_Data = json["m_Data"].asString();
	if (json.isMember("m_Arithmetic"))
		m_Arithmetic = json["m_Arithmetic"].asString();
	if (json.isMember("m_Key"))
		m_Key = json["m_Key"].asString();
}