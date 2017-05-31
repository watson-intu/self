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


#ifndef SELF_LASERDATA_H
#define SELF_LASERDATA_H

#include "IData.h"
#include "SelfLib.h"

class SELF_API LaserData : public IData
{
public:
	RTTI_DECL();

	LaserData() : 
		m_fDistance( 0.0f ), 
		m_fAzimuthalAngle( 0.0f ), 
		m_fElevationAngle( 0.0f )
	{}
	LaserData(float a_Distance, float a_AzimuthalAngle, float a_ElevationAngle) : 
		m_fDistance(a_Distance),
		m_fAzimuthalAngle(a_AzimuthalAngle),
		m_fElevationAngle(a_ElevationAngle)
	{}

	~LaserData()
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json)
	{
		json["m_fDistance"] = m_fDistance;
		json["m_fAzimuthalAngle"] = m_fAzimuthalAngle;
		json["m_fElevationAngle"] = m_fElevationAngle;
	}
	virtual void Deserialize(const Json::Value & json)
	{
		m_fDistance = json["m_fDistance"].asFloat();
		m_fAzimuthalAngle = json["m_fAzimuthalAngle"].asFloat();
		m_fElevationAngle = json["m_fElevationAngle"].asFloat();
	}

	//!Accessors
	float GetDistance() const
	{
		return m_fDistance;
	}
	float GetAzimuthalAngle() const
	{
		return m_fAzimuthalAngle;
	}
	float GetElevationAngle() const
	{
		return m_fElevationAngle;
	}
private:
	//!Data
	float		m_fDistance;
	float		m_fAzimuthalAngle;	
	float		m_fElevationAngle;
};

#endif //SELF_LASERATA_H
