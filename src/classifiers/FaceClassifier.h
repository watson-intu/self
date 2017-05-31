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
#include "blackboard/Person.h"
#include "blackboard/Image.h"
#include "SelfLib.h"

class SelfInstance;
class Object;
class TiXmlDocument;

//! This classifier subscribes to all video sensors and classifies all incoming video data
//! which then adds a Concept to the BlackBoard.
class SELF_API FaceClassifier : public IClassifier
{
public:
	RTTI_DECL();

	//! Types 
	typedef boost::shared_ptr<FaceClassifier>			SP;
	typedef boost::weak_ptr<FaceClassifier>				WP;

	//! Construction
	FaceClassifier();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IClassifier interface
	virtual const char * GetName() const;
	virtual bool OnStart();
	virtual bool OnStop();

	bool LearnPerson( const Person::SP & a_spPerson, const std::string & a_Name );

private:
	//! Types
	struct SensorGroup : public boost::enable_shared_from_this<SensorGroup>
	{
		typedef boost::shared_ptr<SensorGroup>		SP;

		SensorGroup( const FaceClassifier::SP & a_pClassifier ) : m_pClassifier( a_pClassifier ), m_nActive( 0 )
		{}

		bool ProcessPerson( const Image::SP & a_spImage, const Person::SP & a_spPerson );

		FaceClassifier::SP	m_pClassifier;
		Image::SP			m_spImage;
		int					m_nActive;
	};
	typedef std::map<std::string,SensorGroup::SP>	ProcessingMap;

	struct ClassifyFace : public boost::enable_shared_from_this<ClassifyFace>
	{
		typedef boost::shared_ptr<ClassifyFace>		SP;

		ClassifyFace( const SensorGroup::SP & a_pGroup, const Person::SP & a_spPerson );
		~ClassifyFace();

		void OnFaceClassified( const TiXmlDocument & a_F256 );
		void OnFaceFound( const Json::Value & a_Face );

		//! Data
		SP					m_spThis;
		SensorGroup::SP		m_pGroup;
		Person::SP			m_spPerson;
	};

	//! Data
	ProcessingMap	m_ProcessingMap;
	float			m_MinFaceConfidence;

	//! Callbacks
	void OnPerson(const ThingEvent & a_ThingEvent);
	void OnLearnPerson( const Json::Value & a_Response );

};

#endif //SELF_PERSONCLASSIFIER_H
