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


#ifndef OBJECT_CLASSIFIER_H
#define OBJECT_CLASSIFIER_H

#include "IClassifier.h"
#include "blackboard/ThingEvent.h"
#include "blackboard/DepthImage.h"
#include "utils/IService.h"
#include "SelfLib.h"

class SelfInstance;
class IObjectRecognition;

class SELF_API ObjectClassifier : public IClassifier
{
public:
	RTTI_DECL();

	ObjectClassifier()
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IClassifier interface
	virtual const char * GetName() const;
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Types
	struct ClassifyDepthImage : public boost::enable_shared_from_this<ClassifyDepthImage>
	{
		typedef boost::shared_ptr<ClassifyDepthImage>		SP;

		ClassifyDepthImage()
		{}

		DepthImage::SP			m_spDepthImage;

		bool ProcessDepthImage(const DepthImage::SP & a_spImage);
		void OnDepthImageClassified(const Json::Value & json);
	};
	typedef std::map< std::string, ClassifyDepthImage::SP >		ProcessingMap;
	//! Callback handler
	void OnDepthImage(const ThingEvent & a_ThingEvent);

	//! Data
	ProcessingMap			m_ProcessingMap;
}; 

#endif // OBJECT_CLASSIFIER_H
