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


#include "ObjectClassifier.h"
#include "SelfInstance.h"

#include "blackboard/BlackBoard.h"
#include "services/IObjectRecognition.h"

REG_SERIALIZABLE(ObjectClassifier);
RTTI_IMPL(ObjectClassifier, IClassifier);

void ObjectClassifier::Serialize(Json::Value & json)
{
	IClassifier::Serialize(json);
}

void ObjectClassifier::Deserialize(const Json::Value & json)
{
	IClassifier::Deserialize(json);
}

const char * ObjectClassifier::GetName() const
{
	return "ObjectClassifier";
}

bool ObjectClassifier::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert(pInstance != NULL);

	IObjectRecognition * pObjectRecognition = pInstance->FindService<IObjectRecognition>();
	if ( pObjectRecognition == NULL )
	{
		Log::Error( "ObjectClassifier", "Failed to find IObjectRecognition service." );
		return false;
	}

	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert(pBlackboard != NULL);

	pBlackboard->SubscribeToType("DepthImage",
		DELEGATE(ObjectClassifier, OnDepthImage, const ThingEvent &, this), TE_ADDED);

	Log::Status("ObjectClassifier", "ObjectClassifier started");
	return true;
}

bool ObjectClassifier::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert(pInstance != NULL);
	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert(pBlackboard != NULL);

	pBlackboard->UnsubscribeFromType("DepthImage", this);

	Log::Status("ObjectClassifier", "Object Classifier stopped");
	return true;
}

void ObjectClassifier::OnDepthImage(const ThingEvent & a_ThingEvent)
{
	DepthImage::SP spDepthImage = DynamicCast<DepthImage>(a_ThingEvent.GetIThing());
	if (spDepthImage)
	{
		if (m_ProcessingMap.find(spDepthImage->GetOrigin()) == m_ProcessingMap.end())
			m_ProcessingMap[spDepthImage->GetOrigin()] = ClassifyDepthImage::SP( new ClassifyDepthImage() );

		m_ProcessingMap[spDepthImage->GetOrigin()]->ProcessDepthImage(spDepthImage);
	}
}

bool ObjectClassifier::ClassifyDepthImage::ProcessDepthImage(const DepthImage::SP & a_spDepthImage)
{
	if (m_spDepthImage)
		return false;

	IObjectRecognition * pObjectRecognition = Config::Instance()->FindService<IObjectRecognition>();
	if ( pObjectRecognition != NULL && pObjectRecognition->IsConfigured() )
	{
		m_spDepthImage = a_spDepthImage;

		pObjectRecognition->ClassifyObjects(m_spDepthImage->GetContent(),
			DELEGATE(ClassifyDepthImage, OnDepthImageClassified, const Json::Value &, shared_from_this() ));

		return true;
	}

	return false;
}

void ObjectClassifier::ClassifyDepthImage::OnDepthImageClassified(const Json::Value & json)
{
	if (!json.isNull() && json.isMember("objects"))
	{
		Log::Debug("ObjectClassifier", "Classified Depth Image! %s", json.toStyledString().c_str());
		const Json::Value & objects = json["objects"];
		for (size_t i = 0; i < objects.size(); ++i)
		{
			const Json::Value & object = objects[i];

			IThing::SP spThing(new IThing(TT_PERCEPTION, "RecognizedObject", object));
			m_spDepthImage->AddChild(spThing);
		}
	}
	else
	{
		Log::Error("ObjectClassifier", "Invalid response from IObjectRecognition");
	}

	m_spDepthImage.reset();
}