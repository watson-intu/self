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


#include "Network.h"

REG_SERIALIZABLE( Network );
RTTI_IMPL(Network, ISensor);

void Network::Serialize(Json::Value & json)
{
    ISensor::Serialize( json );

    SerializeVector( "m_Addresses", m_Addresses, json );
    json["m_NetworkCheckInterval"] = m_NetworkCheckInterval;
	json["m_fCheckTimeout"] = m_fCheckTimeout;
}

void Network::Deserialize(const Json::Value & json)
{
    ISensor::Deserialize( json );

    DeserializeVector( "m_Addresses", json, m_Addresses );
	if ( json.isMember("m_NetworkCheckInterval") )
		m_NetworkCheckInterval = json["m_NetworkCheckInterval"].asInt();
	if ( json.isMember( "m_fCheckTimeout" ) )
		m_fCheckTimeout = json["m_fCheckTimeout"].asFloat();
}

bool Network::OnStart()
{
    Log::Debug( "Network", "OnStart() invoked." );
	m_NetworkCheckTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(Network, OnNetworkCheck, this),
	                                                        m_NetworkCheckInterval, true, true);
	if (m_Addresses.size() == 0)
		m_Addresses.push_back("www.ibm.com");

	m_bProcessing = false;
	return true;
}

void Network::OnNetworkCheck( )
{
	if (!m_bProcessing)
	{
		m_bProcessing = true;

		m_RequestsCompleted = 0;
		m_RequestsFailed = 0;
		m_RequestsPending = 0;

		for (size_t i=0;i<m_Addresses.size();++i)
		{
			m_RequestsPending += 1;
			//Log::Status( "Network", "Pinging %s", m_Addresses[i].c_str() );
			new Request( m_Addresses[i], DELEGATE(Network, OnResponse, bool, this), m_fCheckTimeout );
		}
	}
}

void Network::OnResponse( bool a_bSuccess )
{
	if ( !a_bSuccess )
		m_RequestsFailed += 1;

	m_RequestsCompleted += 1;

	if ( m_RequestsPending == m_RequestsCompleted )
	{
		m_bProcessing = false;

		if (m_RequestsFailed < m_RequestsCompleted) // network up
		{
			if (! m_bNetworkUp )
			{
				BuildHealthData("UP", false, false);
				m_bNetworkUp = true;
			}
		}
		else // network down
		{
			if (Time().GetEpochTime() - m_LastNetworkDown.GetEpochTime() < m_NetworkCheckInterval * 2) 
			{
				if ( m_bNetworkUp )
				{
					BuildHealthData("DOWN", true, true);
					m_bNetworkUp = false;
				}
			}
			m_LastNetworkDown = Time();
		}
	}
}

Network::Request::Request( const std::string & a_URL, Delegate<bool> a_Callback, float a_fTimeout ) :
		m_spClient( IWebClient::Create(a_URL) ),
		m_Callback(a_Callback),
		m_bDelete(false)
{
	m_spClient->SetRequestType( "GET" );
	m_spClient->SetStateReceiver( DELEGATE(Network::Request, OnState, IWebClient *, this) );
	m_spClient->SetDataReceiver( DELEGATE(Network::Request, OnResponse, IWebClient::RequestData *, this) );

	if (! m_spClient->Send() )
	{
		Log::Error("Network", "Failed to send web request.");
		ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(Request, OnLocalResponse, this));
	}
	else if ( TimerPool::Instance() != NULL )
	{
		m_spTimeoutTimer = TimerPool::Instance()->StartTimer(
				VOID_DELEGATE( Request, OnTimeout, this ), a_fTimeout, true, false );
	}
	else
	{
		Log::Warning( "Network", "No TimerPool instance, timeouts are disabled." );
	}
}

void Network::Request::OnLocalResponse( )
{
	if (m_Callback.IsValid())
	{
		m_Callback( false );
		m_Callback.Reset();
	}
	delete this;
}

void Network::Request::OnState( IWebClient * a_pClient )
{
	//Log::Status( "Network", "OnState() %d", a_pClient->GetState() );
	if ( a_pClient->GetState() == IWebClient::CLOSED )
	{
		Log::Debug( "Network", "Request closed, delete this." );
		m_bDelete = true;
	}
	else if ( a_pClient->GetState() == IWebClient::DISCONNECTED )
	{
		Log::Error( "Network", "Request failed to connect." );
		m_bDelete = true;
		if ( m_Callback.IsValid() )
		{
			m_Callback( false );
			m_Callback.Reset();
		}
	}

	// delete needs to be the last thing we do..
	if ( m_bDelete )
		delete this;
}

void Network::Request::OnResponse( IWebClient::RequestData * a_pResponse )
{
	if ( a_pResponse->m_bDone )
	{
		//Log::Status( "Network", "OnResponse(), done = %s", a_pResponse->m_bDone ? "true" : "false" );
		if ( m_Callback.IsValid() )
		{
			m_Callback( true );
			m_Callback.Reset();
		}
		delete this;
	}
}

void Network::Request::OnTimeout( )
{
	Log::Error( "Network", "OnTimeout() %s", m_spClient->GetURL().GetURL().c_str() );
	m_spTimeoutTimer.reset();

	// closing will call OnState() which will actually take care of deleteing this object.
	if ( m_spClient->Close() )
	{
		if (m_Callback.IsValid())
		{
			m_Callback( false );
			m_Callback.Reset();
		}
	}
}

void Network::BuildHealthData( const char * state, bool error, bool raiseAlert )
{
	Json::Value paramsMap;
	paramsMap["state"] = StringUtil::Format(state);
	paramsMap["error"] = error;
	paramsMap["raiseAlert"] = raiseAlert;
	ThreadPool::Instance()->InvokeOnMain(DELEGATE(Network, SendHealthData, HealthData * , this),
	                                     new HealthData("RemoteNetwork", paramsMap));
}

void Network::SendHealthData(HealthData * a_pData)
{
	SendData(a_pData);
}

bool Network::OnStop()
{
    Log::Debug( "Network", "OnStop() invoked." );
	m_NetworkCheckTimer.reset();
	return true;
}

void Network::OnPause()
{}

void Network::OnResume()
{}

