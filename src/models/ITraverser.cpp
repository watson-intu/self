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


#include "ITraverser.h"
#include "IGraph.h"

RTTI_IMPL( ITraverser, ISerializable );

const NullCondition ITraverser::NULL_CONDITION;

void ITraverser::Serialize(Json::Value & json)
{
	if (m_spCondition)
		json["m_spCondition"] = ISerializable::SerializeObject(m_spCondition.get());
	if (m_spNext)
		json["m_spNext"] = ISerializable::SerializeObject(m_spNext.get());
}

void ITraverser::Deserialize(const Json::Value & json)
{
	if (json.isMember("m_spCondition"))
		m_spCondition = IConditional::SP(ISerializable::DeserializeObject<IConditional>(json["m_spCondition"]));
	if (json.isMember("m_spNext"))
		m_spNext = ITraverser::SP(ISerializable::DeserializeObject<ITraverser>(json["m_spNext"]));
}

