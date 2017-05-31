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


#include "WorldAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"

REG_SERIALIZABLE(WorldAgent);
RTTI_IMPL(WorldAgent, IAgent);

WorldAgent::WorldAgent()
{}

void WorldAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
}

void WorldAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
}

bool WorldAgent::OnStart()
{
	//SelfInstance * pInstance = SelfInstance::GetInstance();
	//assert( pInstance != NULL );
	//BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	//assert( pBlackboard != NULL );

	//pBlackboard->SubscribeToType( "RecognizedFace", DELEGATE( WorldAgent, OnRecognizedFace, const ThingEvent &, this ) );

	return true;
}

bool WorldAgent::OnStop()
{
	//SelfInstance * pInstance = SelfInstance::GetInstance();
	//assert( pInstance != NULL );
	//BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	//assert( pBlackboard != NULL );

	//pBlackboard->UnsubscribeFromType( "RecognizedFace",  this );

	return true;
}

//-----------------------------------------------------------

