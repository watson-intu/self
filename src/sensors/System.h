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


#ifndef SELF_SYSTEM_H
#define SELF_SYSTEM_H

#include "SelfInstance.h"
#include "ISensor.h"
#include "HealthData.h"

#include "utils/TimerPool.h"
#include "utils/Time.h"

#include "SelfLib.h"

//! Base class for a System sensor class
class SELF_API System : public ISensor
{
public:
	RTTI_DECL();

	System( ) : 
		ISensor( "System" ),
		m_SystemCheckInterval( 60 ), 
		m_bProcessing( false ),
		m_fFreeMemoryThreshold( 50.0 ),
	    m_cpuUsageCommand( "top -b -n1 | grep 'Cpu(s)' | awk '{print $2 + $4}' " ),
	    m_diskUsageCommand( "df --total -k -h | tail -1 | awk '{print $5}'"),
	    m_freeMemoryCommand( "free | awk 'FNR == 3 {print $4/($3+$4)*100}'" ),
	    m_lastRebootCommand( "last reboot -F | head -1 | awk '{print $5,$6,$7,$8,$9}'" )
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! ISensor interface
	virtual const char * GetDataType()
	{
		return HealthData::GetStaticRTTI().GetName().c_str();
	}

	virtual bool OnStart();
	virtual bool OnStop();
	virtual void OnPause();
	virtual void OnResume();

	void SetSystemCheckIntervval( float a_fIntervalInSeconds )
	{
		m_SystemCheckInterval = a_fIntervalInSeconds;
	}

protected:
	//! Data
	float                       m_SystemCheckInterval;
	TimerPool::ITimer::SP       m_SystemCheckTimer;
	volatile bool               m_bProcessing;
	std::string                 m_cpuUsageCommand;
	std::string                 m_diskUsageCommand;
	std::string                 m_freeMemoryCommand;
	std::string                 m_lastRebootCommand;
	float                       m_fFreeMemoryThreshold;

	void                        SendHealthData( HealthData * a_pData );
	void                        OnCheckSystem( );
	void                        DoOnCheckSystem();
	std::string                 ExecuteCommand( const std::string& command );
	void                        BuildHealthData( const char * state, float value, bool error, bool raiseAlert );
};

#endif	// SELF_SYSTEM_H