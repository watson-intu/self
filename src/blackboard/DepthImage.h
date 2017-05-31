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


#ifndef SELF_DEPTHIMAGE_H
#define SELF_DEPTHIMAGE_H

#include "IThing.h"

//! A depth image object taken from 3d camera sensor
class SELF_API DepthImage : public IThing
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<DepthImage>			SP;
	typedef boost::weak_ptr<DepthImage>				WP;

	//! Construction
	DepthImage() : IThing(TT_PERCEPTION, 30.0f)
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Returns a buffer containing the RGB-D image.
	const std::string & GetContent() const
	{
		return m_Content;
	}
	//! Set the RGB-D image.
	void SetContent(const std::string & a_Content)
	{
		m_Content = a_Content;
	}

	//! Returns the GUID of the sensor.
	const std::string & GetOrigin() const
	{
		return m_Origin;
	}
	//! Set the origin for this depth image, this should always be the GUID of the sensor.
	void SetOrigin(const std::string & a_Origin)
	{
		m_Origin = a_Origin;
	}

private:
	//! Data
	std::string m_Content;
	std::string	m_Origin;
};

#endif //SELF_DEPTHIMAGE_H
