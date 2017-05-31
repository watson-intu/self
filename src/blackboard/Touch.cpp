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


#include "Touch.h"

REG_SERIALIZABLE(Touch);
RTTI_IMPL(Touch, IThing);

void Touch::Serialize(Json::Value & json)
{
	IThing::Serialize(json);

	json["m_SensorName"] = m_SensorName;
	json["m_fEngaged"] = m_fEngaged;
}

void Touch::Deserialize(const Json::Value & json)
{
	IThing::Deserialize(json);

	m_SensorName = json["m_SensorName"].asString();
	m_fEngaged = json["m_fEngaged"].asFloat();
}

