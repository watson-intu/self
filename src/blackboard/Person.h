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


#ifndef PERSON_H
#define PERSON_H

#include "IThing.h"
#include "models/IGraph.h"

//! A person as recognized by the visual recognition system.
class SELF_API Person : public IThing
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<Person>	SP;
	typedef boost::weak_ptr<Person>		WP;
	typedef std::list<std::string> StringList;

	//! Construction
	Person();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Accessors
	const std::string & GetGender() const
	{
		return m_Gender;
	}
	const std::string & GetAgeRange() const
	{
		return m_AgeRange;
	}
	const std::vector<float> & GetFaceLocation() const
	{
		return m_FaceLocation;
	}
	const std::string & GetFaceImage() const
	{
		return m_FaceImage;
	}
	
	//! Mutators
	void SetGender( const std::string & a_Gender )
	{
		m_Gender = a_Gender;
	}
	void SetAgeRange( const std::string & a_Age)
	{
		m_AgeRange = a_Age;
	}
	void SetFaceLocation( const std::vector<float> & a_Location )
	{
		m_FaceLocation = a_Location;
	}
	void SetFaceImage( const std::string & a_Image )
	{
		m_FaceImage = a_Image;
	}

	//! Distance of this face from the center of the camera, this is used to determine who we should
	//! be giving focus..
	float FaceDistance() const
	{
		float sq = 0.0f;
		for(size_t i=0;i<m_FaceLocation.size();++i)
			sq += m_FaceLocation[i] * m_FaceLocation[i];

		return sqrt( sq );
	}

	//! Returns the GUID of the sensor.
	const std::string & GetOrigin() const
	{
		return m_Origin;
	}
	//! Set the origin for this image, this should always be the GUID of the sensor.
	void SetOrigin(const std::string & a_Origin)
	{
		m_Origin = a_Origin;
	}

private:
	//! Data
	std::string m_Gender;			// detected gender & age from face detection
	std::string m_AgeRange;
	std::vector<float>
				m_FaceLocation;		// location of the face in the field of view normalized
	std::string	m_FaceImage;		// the last image of the face

	std::string	m_Origin;           // sensor GUID
};

#endif
