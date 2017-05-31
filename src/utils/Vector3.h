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


#ifndef VECTOR3_H
#define VECTOR3_H

#include <string>
#include <stdio.h>

//! This class is used to contain a 3D position in space.
class Vector3
{
public:
	//! Construction
	Vector3() : m_X( 0.0f ), m_Y( 0.0f ), m_Z( 0.0f )
	{}
	Vector3( const Vector3 & a_Copy ) : 
		m_X( a_Copy.m_X ),
		m_Y( a_Copy.m_Y ),
		m_Z( a_Copy.m_Z )
	{}
	Vector3( float x, float y, float z )
	{
	}
	Vector3( const std::string & a_Parse )
	{
		sscanf( a_Parse.c_str(), "%g,%g,%g", &m_X, &m_Y, &m_Z );
	}

	std::string ToString() const
	{
		char buffer[ 256 ];
		snprintf( buffer, sizeof(buffer), "%g,%g,%g", m_X, m_Y, m_Z );
	}

	float GetX() const
	{
		return m_X;
	}
	float GetY() const
	{
		return m_Y;
	}
	float GetZ() const
	{
		return m_Z;
	}

private:
	//! Data
	float	m_X;
	float	m_Y;
	float	m_Z;
};

#endif

