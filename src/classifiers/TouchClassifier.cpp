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


#include "TouchClassifier.h"

REG_SERIALIZABLE(TouchClassifier);
RTTI_IMPL(TouchClassifier, IClassifier);

void TouchClassifier::Serialize(Json::Value & json)
{
	IClassifier::Serialize(json);
}

void TouchClassifier::Deserialize(const Json::Value & json)
{
	IClassifier::Deserialize(json);
}

const char * TouchClassifier::GetName() const
{
	return "TouchClassifier";
}

bool TouchClassifier::OnStart()
{
	BlackBoard * pBlackboard = SelfInstance::GetInstance()->GetBlackBoard();
	pBlackboard->SubscribeToType("Touch",
		DELEGATE(TouchClassifier, OnTouch, const ThingEvent &, this), TE_ADDED);
	Log::Status("TouchClassifier", "TouchClassifier started");
	return true;
}

bool TouchClassifier::OnStop()
{
	Log::Status("TouchClassifier", "Touch Classifier stopped");
	BlackBoard * pBlackboard = SelfInstance::GetInstance()->GetBlackBoard();
	pBlackboard->UnsubscribeFromType("Touch", this);
	return true;
}

void TouchClassifier::OnTouch(const ThingEvent & a_ThingEvent)
{
	Touch::SP spTouch = DynamicCast<Touch>(a_ThingEvent.GetIThing());

	if (spTouch)
	{
		if(spTouch->IsEngaged())
		{
			Log::Debug("TouchClassifier", "Received Touch Event!!");
			spTouch->AddChild(IThing::SP(
					new IThing(TT_PERCEPTION, "TouchType", IThing::JsonObject("m_TouchType", spTouch->GetSensorName()), 3600.0f)));
		}
	}
}
