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


#ifndef SELF_IFACECLASSIFIER_H
#define SELF_IFACECLASSIFIER_H

#include "utils/IService.h"
#include "SelfLib.h"

//! Face recognition service interface
class SELF_API IFaceRecognition : public IService
{
public:
	RTTI_DECL();

	//! Construction
	IFaceRecognition( const std::string & a_ServiceId, AuthType a_AuthType = AUTH_BASIC ) : 
		IService( a_ServiceId, a_AuthType )
	{}

	//! Classify the provided face image data, should return a response containing the F256 data for the provided image
	virtual void ClassifyFace( const std::string & a_FaceImageData,
		Delegate<const TiXmlDocument &> a_Callback ) = 0;
	//! Search for a face in the DB with the given F256 profile..
	virtual bool SearchForFace( const TiXmlDocument & a_F256,
		float a_fThreshold, int a_MaxResults,
		Delegate<const Json::Value &> a_Callback ) = 0;
	//! Upload a new face along with a name
	virtual bool AddFace( const TiXmlDocument & a_F256,
		const std::string & a_FaceImage,
		const std::string & a_PersonId,
		const std::string & a_Name,
		const std::string & a_Gender,
		const std::string & a_DOB,
		Delegate<const Json::Value &> a_Callback ) = 0;
};

#endif //SELF_IFACECLASSIFIER_H
