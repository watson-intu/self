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


#include "LocalSpeechToText.h"

REG_SERIALIZABLE( LocalSpeechToText );
RTTI_IMPL(LocalSpeechToText, ISensor);

void LocalSpeechToText::Serialize(Json::Value & json)
{
	ISensor::Serialize(json);
    SerializeVector("m_VocabularyList", m_VocabularyList, json);
    json["m_MinConfidence"] = m_MinConfidence;
}

void LocalSpeechToText::Deserialize(const Json::Value & json)
{
	ISensor::Deserialize(json);
    DeserializeVector("m_VocabularyList", json, m_VocabularyList);
    m_MinConfidence = json["m_MinConfidence"].asFloat();
}

bool LocalSpeechToText::OnStart()
{
    return true;
}

bool LocalSpeechToText::OnStop()
{
    return true;
}

void LocalSpeechToText::OnPause()
{}

void LocalSpeechToText::OnResume()
{}