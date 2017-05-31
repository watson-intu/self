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


#ifndef OBSTACLE_H
#define OBSTACLE_H

#include "IThing.h"
#include "utils/Vector3.h"

//! This thing represents an obstruction/obstacle in the environment to be avoided.
class SELF_API Obstacle : public IThing
{
public:
	RTTI_DECL();

	Obstacle() : IThing( TT_PERCEPTION )
	{}

	const Vector3 & GetMin() const
	{
		return m_Min;
	}
	const Vector3 & GetMax() const
	{
		return m_Max;
	}

	void SetBounds( const Vector3 & a_Min, const Vector3 & a_Max )
	{
		m_Min = a_Min;
		m_Max = a_Max;
	}

private:
	//! Data
	Vector3		m_Min, m_Max;
};

#endif
