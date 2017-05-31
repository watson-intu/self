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


#ifndef SELF_ILOCATION_H
#define SELF_ILOCATION_H

#include "utils/IService.h"
#include "SelfLib.h"

struct Location;

//! Service wrapper for location data 
class SELF_API ILocation : public IService
{
public:
	RTTI_DECL();

	//! Types
	typedef Delegate<const Json::Value &>			SendCallback;

	//! Construction
	ILocation( const std::string & a_ServiceId, AuthType a_AuthType = AUTH_BASIC ) : IService( a_ServiceId, a_AuthType )
	{}

	virtual void GetLocation( const std::string & a_Location,
		Delegate<const Json::Value &> a_Callback) = 0;
	virtual void GetTimeZone(const double & a_Latitude, const double & a_Longitude, 
		Delegate<const Json::Value &> a_Callback) = 0;
};


struct SELF_API Location : public boost::enable_shared_from_this<Location>
{
	//! Types
	typedef boost::shared_ptr<Location>			SP;
	typedef boost::weak_ptr<Location>			WP;

	Location(const std::string & a_LocationName, float a_Latitude, float a_Longitude) :
			m_LocationName(a_LocationName),
			m_Latitude(a_Latitude),
			m_Longitude(a_Longitude)
	{}

	std::string	m_LocationName;
	float		m_Longitude;
	float		m_Latitude;

	const std::string & GetLocationName() const
	{
		return m_LocationName;
	}

	float GetLongitude() const
	{
		return m_Longitude;
	}

	float GetLatitude() const
	{
		return m_Latitude;
	}
};

#endif
