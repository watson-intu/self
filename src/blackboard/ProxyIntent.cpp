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


#include "ProxyIntent.h"

REG_SERIALIZABLE(ProxyIntent);
RTTI_IMPL(ProxyIntent, IIntent);

void ProxyIntent::Serialize(Json::Value &json)
{
	IIntent::Serialize(json);
}

void ProxyIntent::Deserialize(const Json::Value &json)
{
	IIntent::Deserialize(json);
}

void ProxyIntent::Create(const Json::Value & a_Intent, const Json::Value & a_Parse)
{
	IIntent::Create(a_Intent, a_Parse);
}