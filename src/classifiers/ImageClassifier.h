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


#ifndef IMAGE_CLASSIFIER_H
#define IMAGE_CLASSIFIER_H

#include <list>

#include "IClassifier.h"
#include "blackboard/ThingEvent.h"
#include "blackboard/Image.h"
#include "utils/IService.h"
#include "utils/DataCache.h"
#include "SelfLib.h"

class SelfInstance;
class IVisualRecognition;

//! This classifier subscribes to all video sensors and classifies all incoming video data
//! which then adds a Concept to the BlackBoard.
class SELF_API ImageClassifier : public IClassifier
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<ImageClassifier>		SP;
	typedef boost::weak_ptr<ImageClassifier>		WP;

	//! Construction
	ImageClassifier() : 
		m_MinClassifyConfidence( 0.4 ),
		m_MaxCacheSize( (1024 * 1024) * 100 ),		// default to 100mb
		m_ServiceId( "VisualRecognitionV1" ), 
		m_ClassifierName( "self" ),
		m_bForceRetrain(false),
		m_fRestartTime( 300.0f ),
		m_bUseDefaultClassifier( true ),
		m_nPendingOps( 0 )
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IClassifier interface
	virtual const char * GetName() const;
	virtual bool OnStart();
	virtual bool OnStop();

	void SubmitPositiveImages( const std::list<Image::SP> & a_Images, const std::string & a_Class);
	void SubmitNegativeImages( const std::list<Image::SP> & a_Images, const std::string & a_Class);

private:
	//! Types
	typedef std::vector< std::string >	FileVector;
	typedef std::list< std::string >	FileList;

	struct ClassifyImage : public boost::enable_shared_from_this<ClassifyImage>
	{
		typedef boost::shared_ptr<ClassifyImage>		SP;

		ClassifyImage( const ImageClassifier::SP & a_spClassifier ) : m_pClassifier( a_spClassifier )
		{}

		ImageClassifier::SP	m_pClassifier;
		Image::SP			m_spImage;

		bool ProcessImage( const Image::SP & a_spImage );
		void OnImageClassified(const Json::Value & json);
	};
	typedef std::map< std::string, ClassifyImage::SP >		ProcessingMap;

	//! Data
	double					m_MinClassifyConfidence;	// confidence before an entity is put on the blackboard
	unsigned int			m_MaxCacheSize;				// maximum size of our images cache in bytes
	std::string				m_ServiceId;
	std::string				m_ClassifierName;
	bool					m_bForceRetrain;

	std::string 			m_ClassifierId;
	std::string				m_ClassifierDate;
	float					m_fRestartTime;
	bool					m_bUseDefaultClassifier;
	FileList				m_PendingExamples;		// new examples to update classifier with

	DataCache				m_ImagesCache;
	ProcessingMap			m_ProcessingMap;
	TimerPool::ITimer::SP	m_spRestartTimer;

	TimerPool::ITimer::SP	m_spRetrainingTimer;
	int						m_nPendingOps;

	//! Callback handler
	void OnGetClassifiers(const Json::Value & a_Response );
	void OnGetClassifier(const Json::Value & a_Response );
	void OnDeleteClassifier( IService::Request * a_Response );
	void OnImage(const ThingEvent & a_ThingEvent);
	void OnHealth(const ThingEvent & a_Event);

	void RestartClassifier();

	void UpdateClassifier( const std::string & a_NewExample );
	void InitializeClassifier();
	void OnCreateClassifier( const Json::Value & a_Response );
	void UpdateClassifier();
	void OnUpdateClassifier( const Json::Value & a_Response );
	void CheckTrainingClassifier();
	void OnCheckClassifier( const Json::Value & a_Response );
};

#endif

