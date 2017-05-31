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


#include "DiscoveryAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"	// BlackBoard
#include "utils/WebClientService.h"	// WebClientService
#include "topics/TopicManager.h"

#define RECV_BUFFER_SIZE 1024

REG_SERIALIZABLE(DiscoveryAgent);
RTTI_IMPL(DiscoveryAgent, IAgent);

DiscoveryAgent::DiscoveryAgent() :
	m_fDiscoveryInterval(0.0f), m_Port(9444), m_MulticastAddress("239.255.0.1"), m_MulticastTTL(5), m_pSocket(NULL)
{}

void DiscoveryAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
	json["m_fDiscoveryInterval"] = m_fDiscoveryInterval;
	json["m_Port"] = m_Port;
	json["m_MulticastAddress"] = m_MulticastAddress;
	json["m_MulticastTTL"] = m_MulticastTTL;
}

void DiscoveryAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
	if (json["m_fDiscoveryInterval"].isDouble())
		m_fDiscoveryInterval = json["m_fDiscoveryInterval"].asFloat();
	if (json["m_Port"].isInt())
		m_Port = json["m_Port"].asInt();
	if (json["m_MulticastAddress"].isString())
		m_MulticastAddress = json["m_MulticastAddress"].asString();
	if (json["m_MulticastTTL"].isInt())
		m_MulticastTTL = json["m_MulticastTTL"].asInt();
}

bool DiscoveryAgent::OnStart()
{
	Log::Debug("DiscoveryAgent", "DiscoveryAgent has started!");

	m_ListenEndpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::any(), m_Port);

	m_pSocket = new boost::asio::ip::udp::socket(WebClientService::Instance()->GetService());
	try {
		m_pSocket->open(m_ListenEndpoint.protocol());
		m_pSocket->set_option(boost::asio::ip::udp::socket::reuse_address(true));
		m_pSocket->set_option(boost::asio::socket_base::broadcast(true));
		m_pSocket->bind(m_ListenEndpoint);
		m_pSocket->set_option(boost::asio::ip::multicast::join_group(boost::asio::ip::address::from_string(m_MulticastAddress)));
		m_pSocket->set_option(boost::asio::ip::multicast::hops(m_MulticastTTL));
	}
	catch (const std::exception & ex)
	{
		Log::Error("DiscoveryAgent", "Failed to start: %s", ex.what());
		return false;
	}

	StartRecv();

	if (m_fDiscoveryInterval > 0.0f)
	{
		StartDiscover();

		m_spDiscoveryTimer = TimerPool::Instance()->StartTimer(
			VOID_DELEGATE(DiscoveryAgent, StartDiscover, this),
			m_fDiscoveryInterval, true, true);
	}

	return true;
}

bool DiscoveryAgent::OnStop()
{
	Log::Debug("DiscoveryAgent", "DiscoveryAgent has stopped!");

	m_spDiscoveryTimer.reset();

	delete m_pSocket;
	m_pSocket = NULL;

	return true;
}

void DiscoveryAgent::StartDiscover()
{
	Json::Value message;
	message["action"] = "ping";

	boost::shared_ptr<std::string> spSendBuffer(new std::string(message.toStyledString()));

	boost::asio::ip::udp::endpoint broadcast(boost::asio::ip::address_v4::broadcast(), m_Port);
	m_pSocket->async_send_to(boost::asio::buffer(*spSendBuffer), broadcast,
		boost::bind(&DiscoveryAgent::HandleSend, this, spSendBuffer,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void DiscoveryAgent::HandleRecv(boost::shared_ptr<std::string> a_spRecvBuffer,
	const boost::system::error_code & a_Error, size_t a_BytesRecv)
{
	// try to parse received message, this is either going to be a ping (request for a pong) 
	// or a pong (answer to a ping) message.
	if (!a_Error && a_BytesRecv > 0)
	{
		std::string message(a_spRecvBuffer->data(), a_BytesRecv);

		Json::Value root;
		if (!Json::Reader(Json::Features::strictMode()).parse(message, root))
		{
			Log::Error("DiscoveryAgent", "Failed to parse message: %s", message.c_str());
			root.clear();
		}

		if (root["action"].isString())
		{
			std::string action = root["action"].asString();
			if (action == "ping")
			{
				Json::Value response;
				response["action"] = "pong";

				SelfInstance * pInstance = SelfInstance::GetInstance();
				if (pInstance != NULL)
				{
					response["groupId"] = pInstance->GetLocalConfig().m_GroupId;
					response["orgId"] = pInstance->GetLocalConfig().m_OrgId;
					response["embodimentId"] = pInstance->GetLocalConfig().m_EmbodimentId;
					response["name"] = pInstance->GetLocalConfig().m_Name;
					response["macId"] = pInstance->GetLocalConfig().m_MacId;
					response["type"] = pInstance->GetRobotType();
					response["instanceId"] = pInstance->GetInstanceId();
					response["port"] = pInstance->GetTopicManager()->GetPort();
				}

				boost::shared_ptr<std::string> spSendBuffer(new std::string(response.toStyledString()));

				boost::asio::ip::udp::endpoint broadcast(boost::asio::ip::address_v4::broadcast(), m_Port);
				m_pSocket->async_send_to(boost::asio::buffer(*spSendBuffer), broadcast,
					boost::bind(&DiscoveryAgent::HandleSend, this, spSendBuffer,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
			}
			else if (action == "pong")
			{
				root["ip"] = m_RemoteEndpoint.address().to_string();

				IThing::SP spInstance(new IThing(TT_MODEL, "SelfInstance", root, m_fDiscoveryInterval));
				ThreadPool::Instance()->InvokeOnMain<IThing::SP>(
					DELEGATE(DiscoveryAgent, OnDiscovered, IThing::SP, this), spInstance);
			}
			else
				Log::Warning("DiscoveryAgent", "Unknown action %s received.", action.c_str());
		}
	}

	// receive next packet..
	if (!a_Error)
		StartRecv();
}

void DiscoveryAgent::HandleSend(boost::shared_ptr<std::string> a_spSendBuffer,
	const boost::system::error_code & a_Error, size_t a_BytesRecv)
{
	if (a_Error)
		Log::Error("DiscoveryAgent", "Failed to send packet.");
}

void DiscoveryAgent::StartRecv()
{
	boost::shared_ptr<std::string> spRecvBuffer(new std::string(RECV_BUFFER_SIZE, '\0'));
	m_pSocket->async_receive_from(boost::asio::buffer((char *)spRecvBuffer->data(), spRecvBuffer->size()),
		m_RemoteEndpoint, boost::bind(&DiscoveryAgent::HandleRecv, this, spRecvBuffer,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void DiscoveryAgent::OnDiscovered(IThing::SP a_spDiscovered)
{
	// firstly, search our current list of discovered instances removing any aged instances and duplicates
	// of this incoming instance.
	for (size_t i = 0; i < m_Discovered.size();)
	{
		if (!m_Discovered[i]->HasParent() || ((*m_Discovered[i])["instanceId"] == (*a_spDiscovered)["instanceId"]))
		{
			m_Discovered.erase(m_Discovered.begin() + i);
			continue;
		}
		i += 1;
	}

	m_Discovered.push_back(a_spDiscovered);
	Log::Status("DiscoveryAgent", "OnDiscovered instance %s, IP: %s, %u instances", (*a_spDiscovered)["instanceId"].asCString(),
		(*a_spDiscovered)["ip"].asCString(), m_Discovered.size());

	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance != NULL)
		pInstance->GetBlackBoard()->AddThing(a_spDiscovered);
}

