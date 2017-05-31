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


#ifndef SELF_PERSONCLASSIFIER_H
#define SELF_PERSONCLASSIFIER_H

#include "IClassifier.h"
#include "blackboard/ThingEvent.h"
#include "blackboard/Entity.h"
#include "blackboard/Image.h"
#include "topics/TopicManager.h"
#include "SelfLib.h"

class SelfInstance;
class Object;

namespace cv {
	class CascadeClassifier;
};

//! This classifier subscribes to all video sensors and classifies all incoming video data
//! which then adds a Concept to the BlackBoard.
class SELF_API PersonClassifier : public IClassifier
{
public:
	RTTI_DECL();

	//! Construction
	PersonClassifier();
	
	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IClassifier interface
	virtual const char * GetName() const;
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Types
	struct ClassifyFaces
	{
		//! Types
		typedef boost::shared_ptr<ClassifyFaces>		SP;

		//! Construction
		ClassifyFaces(PersonClassifier * a_pClassifier);

		bool ProcessImage( const Image::SP & a_spImage );		//!< Will return false if busy already
		void OnClassifyFaces();
		void OnClassifyDone( Json::Value a_Result );
		void OnFacesDetected(const Json::Value & json);

		PersonClassifier *		m_pClassifier;
		cv::CascadeClassifier *	m_pFaceClassifier;
		Image::SP				m_spImage;
		double					m_LastClassify;
	};

	typedef std::map<std::string,ClassifyFaces::SP>		ProcessingMap;

	//! Data
	std::string		m_ServiceId;
	std::string		m_PersonClass;
	std::string		m_FaceClassifierFile;
	double			m_DetectFacesInterval;		//!< How often to detect faces in the image in seconds per sensor
	float			m_Padding;					//!< Percentage of padding around the detected face to add

	ProcessingMap	m_ProcessingMap;
	int				m_nPeopleSubs;

	//! Callbacks
	void OnImage(const ThingEvent & a_ThingEvent);
	void OnPeople(const TopicManager::SubInfo & a_Sub );
};

#endif //SELF_PERSONCLASSIFIER_H
