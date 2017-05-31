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


#ifndef SELF_NETWORK_H
#define SELF_NETWORK_H

#include "SelfInstance.h"
#include "ISensor.h"
#include "HealthData.h"

#include "utils/TimerPool.h"
#include "utils/Time.h"
#include "utils/IWebClient.h"

#include "SelfLib.h"

//! Base class for a Network sensor class
class SELF_API Network : public ISensor
{
public:
    RTTI_DECL();

    Network( ) : ISensor("Network"),
		m_NetworkCheckInterval( 20 ), 
		m_fCheckTimeout( 3.0f ),
		m_bNetworkUp( true ), 
		m_bProcessing( false ),
		m_RequestsCompleted( 0 ),
		m_RequestsPending( 0 ),
		m_RequestsFailed( 0 )
    {}

    //! ISerializable interface
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

	class Request
	{
	public:
		Request( const std::string & a_URL, Delegate<bool> a_Callback, float a_fTimeout );

		virtual ~Request()
		{}

		//! HTTP callbacks
		void OnLocalResponse( );
		void OnState( IWebClient * a_pClient );
		void OnResponse( IWebClient::RequestData * a_pResponse );
		void OnTimeout( );

		//! Data
		IWebClient::SP          m_spClient;
		TimerPool::ITimer::SP	m_spTimeoutTimer;
		Delegate<bool>          m_Callback;
		bool                    m_bDelete;
	};

    //! ISensor interface
    virtual const char * GetDataType()
    {
        return "HealthData";
    }

    virtual bool OnStart();
    virtual bool OnStop();
    virtual void OnPause();
    virtual void OnResume();

	//! Accessors
	bool IsNetworkUp() const
	{
		return m_bNetworkUp;
	}

private:
    //! Data
    std::vector<std::string>    m_Addresses;
    std::string                 m_RemoteIP;
    int                         m_NetworkCheckInterval;
	float						m_fCheckTimeout;
    TimerPool::ITimer::SP       m_NetworkCheckTimer;
	bool						m_bNetworkUp;
	bool                        m_bProcessing;
	Time                        m_LastNetworkDown;

	int                         m_RequestsCompleted;
	int                         m_RequestsPending;
	int                         m_RequestsFailed;

	void                        OnNetworkCheck( );
	void                        OnResponse( bool a_bSuccess );
	void                        SendHealthData( HealthData * a_pData );
	void                        BuildHealthData( const char * state, bool error, bool raiseAlert );
};

#endif	// SELF_NETWORK_H