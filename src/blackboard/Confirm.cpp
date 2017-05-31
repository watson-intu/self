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


#include "Confirm.h"

REG_SERIALIZABLE(Confirm);
RTTI_IMPL(Confirm, IThing);

void Confirm::Serialize(Json::Value & json)
{
	IThing::Serialize(json);
	json["m_bConfirmed"] = m_bConfirmed;
	json["m_ConfirmType"] = m_ConfirmType;
}

void Confirm::Deserialize(const Json::Value & json)
{
	IThing::Deserialize(json);

	if ( json.isMember("m_bConfirmed") )
		m_bConfirmed = json["m_bConfirmed"].asBool();
	if ( json.isMember("m_ConfirmType") )
		m_ConfirmType = json["m_ConfirmType"].asString();
}

void Confirm::OnConfirmed()
{
	m_bConfirmed = true;
	SetState("COMPLETED");
}

void Confirm::OnCancelled()
{
	m_bConfirmed = false;
	SetState("COMPLETED");
}

