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


#ifndef SELF_IVISUALRECOGNITION_H
#define SELF_IVISUALRECOGNITION_H

#include "utils/Delegate.h"
#include "utils/DataCache.h"
#include "utils/IService.h"
#include "SelfLib.h"		// include last always

//! Visual recognition service interface
class SELF_API IVisualRecognition : public IService
{
public:
	RTTI_DECL();

	//! Types
	typedef Delegate<const Json::Value &>	OnGetClassifier;
	typedef Delegate<const Json::Value &>	OnCreateClassifier;
	typedef Delegate<const Json::Value &>	OnClassifyImage;
	typedef Delegate<IService::Request *>	OnDeleteClassifier;
	typedef Delegate<const Json::Value &>	OnDetectFaces;
	typedef Delegate<const Json::Value &>   OnIdentifyText;
	typedef Delegate<const Json::Value &>   OnClassifierTrained;

	//! Construction 
	IVisualRecognition( const std::string & a_ServiceId, AuthType a_AuthType = AUTH_BASIC ) 
		: IService( a_ServiceId, a_AuthType )
	{}

	//! Get a list of classifiers
	virtual void GetClassifiers( OnGetClassifier a_Callback ) = 0;
	//! Check for a classifier
	virtual void GetClassifier( const std::string & a_ClassifierId,
		OnGetClassifier a_Callback ) = 0;
	//! Classify the given image and returns image tags for the image data.
	virtual void ClassifyImage(const std::string & a_ImageData,
		const std::vector<std::string> & a_Classifiers,
		OnClassifyImage a_Callback,
		bool a_bKnowledgeGraph = false ) = 0;
	//! Detect faces in the provided image 
	virtual void DetectFaces(const std::string & a_ImageData,
		OnDetectFaces a_Callback ) = 0;
	//! Create a new custom classifier with the provided negative/positive examples
	virtual void CreateClassifier( const std::string & a_ClassiferName,
		const std::vector<std::string> & a_PositiveExamples,
		const std::string & a_NegativeExamples,
		OnCreateClassifier a_Callback ) = 0;
	//! Retrains the Image Classifier with positive examples
	virtual void UpdateClassifier(
		const std::string & a_ClassifierId,
		const std::string & a_ClassifierName,
		const std::vector< std::string > & a_PositiveExamples,
		const std::string & a_NegativeExamples,
		OnClassifierTrained a_Callback) = 0;
	//! Delete the specified classifier
	virtual void DeleteClassifier( const std::string & a_ClassifierId,
		OnDeleteClassifier a_Callback ) = 0;
};

#endif
