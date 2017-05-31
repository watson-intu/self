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


#ifndef SELF_DISCOVERY_AGENT_H
#define SELF_DISCOVERY_AGENT_H

#include "boost/asio.hpp"		// not including SSL at this level on purpose

#include "IAgent.h"
#include "blackboard/Display.h"
#include "SelfLib.h"

class SELF_API DiscoveryAgent : public IAgent
{
public:
	RTTI_DECL();

	//! Types
	typedef std::vector< IThing::SP >		Instances;

	//! Construction
	DiscoveryAgent();

	//! ISerializable interface
	void Serialize( Json::Value & json );
	void Deserialize( const Json::Value & json );

	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

	//! Accessors
	const Instances & GetDiscovered() const
	{
		return m_Discovered;
	}

	//! Send out a ping to discover all other instances of DiscoveryAgent's running on the local
	//! network. The user should invoke this, then call the GetDiscovered() after some amount of time
	//! to get a list of discovered instances.
	void StartDiscover();

private:
	float				m_fDiscoveryInterval;		// how often in seconds to discover other instances
	int					m_Port;						// what port to listen on 
	std::string			m_MulticastAddress;
	int					m_MulticastTTL;				// time to live setting for multi-cast, how many routes can the packet pass through before getting killed

	Instances			m_Discovered;
	TimerPool::ITimer::SP
						m_spDiscoveryTimer;

	boost::asio::ip::udp::endpoint
						m_ListenEndpoint;
	boost::asio::ip::udp::socket *
						m_pSocket;
	boost::asio::ip::udp::endpoint
						m_RemoteEndpoint;

	void HandleRecv( boost::shared_ptr<std::string> a_spRecvBuffer,
		const boost::system::error_code & a_Error, size_t a_BytesRecv );
	void HandleSend( boost::shared_ptr<std::string> a_spSendBuffer,
		const boost::system::error_code & a_Error, size_t a_BytesRecv );

	void StartRecv();
	void OnDiscovered( IThing::SP a_spDiscovered );
};

#endif //SELF_DISCOVERY_AGENT_H
