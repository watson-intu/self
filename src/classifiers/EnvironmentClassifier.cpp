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


#include "EnvironmentClassifier.h"

REG_SERIALIZABLE(EnvironmentClassifier);
RTTI_IMPL( EnvironmentClassifier, IClassifier);

void EnvironmentClassifier::Serialize(Json::Value & json)
{
    IClassifier::Serialize( json );
    json["m_EnvironmentAnomalyThreshold"] = m_EnvironmentAnomalyThreshold;
}

void EnvironmentClassifier::Deserialize(const Json::Value & json)
{
    IClassifier::Deserialize( json );

    if (json.isMember("m_EnvironmentAnomalyThreshold"))
        m_EnvironmentAnomalyThreshold = json["m_EnvironmentAnomalyThreshold"].asInt();
}

const char * EnvironmentClassifier::GetName() const
{
    return "EnvironmentClassifier";
}

bool EnvironmentClassifier::OnStart()
{
    BlackBoard * pBlackboard = SelfInstance::GetInstance()->GetBlackBoard();
    pBlackboard->SubscribeToType( Environment::GetStaticRTTI(),
                                  DELEGATE( EnvironmentClassifier, OnEnvironment, const ThingEvent &, this ), TE_ADDED );
    Log::Status("EnvironmentClassifier", "EnvironmentClassifier started");
    return true;
}

bool EnvironmentClassifier::OnStop()
{
    Log::Status("EnvironmentClassifier", "Environment Classifier stopped");
    BlackBoard * pBlackboard = SelfInstance::GetInstance()->GetBlackBoard();
    pBlackboard->UnsubscribeFromType( Environment::GetStaticRTTI(), this );
    return true;
}

void EnvironmentClassifier::OnEnvironment(const ThingEvent & a_ThingEvent)
{
    spEnvironment = DynamicCast<Environment>(a_ThingEvent.GetIThing());
    while(m_Environments.size() > 50)
        m_Environments.pop_back();

	// TODO: There has to be a better way then this to classify the environment?
	if (m_Environments.size() > 0)
	{
		Environment::SP spPrevious = m_Environments.front();

        int lastCarbonDioxide = spEnvironment->GetCarbonDioxide();
        int previousCarbonDioxide = spPrevious->GetCarbonDioxide();

        Log::Debug("EnvironmentClassifier", "Comparing CO2 Levels: %d, %d", lastCarbonDioxide, previousCarbonDioxide);
        if( ( lastCarbonDioxide - previousCarbonDioxide ) > m_EnvironmentAnomalyThreshold)
            spEnvironment->SetState("Anomaly_CO2");
    }

	m_Environments.push_front(spEnvironment);
}
