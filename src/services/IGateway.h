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


#ifndef SELF_IGATEWAY_H
#define SELF_IGATEWAY_H

#include "blackboard/ThingEvent.h"
#include "utils/IService.h"
#include "utils/TimerPool.h"
#include "SelfLib.h"			// include last always

struct ServiceList;

//! Interface for a self instance to register with a common gateway for managing instances
class SELF_API IGateway : public IService
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<IGateway>			SP;
	typedef boost::weak_ptr<IGateway>			WP;

	//! Construction
	IGateway( const std::string & a_ServiceId, AuthType a_AuthType = AUTH_BASIC ) : 
		IService( a_ServiceId, a_AuthType )
	{}

	//! Register embodiment on gateway
	virtual void RegisterEmbodiment( Delegate<const Json::Value &> a_Callback ) = 0;
	//! Update the embodiment name
	virtual void UpdateEmbodimentName(Delegate<const Json::Value &> a_Callback) = 0;
	//! Send backtrace from previous crash
	virtual void SendBacktrace( const std::string & a_BT ) = 0;
	//! Send heart-beat to the gateway
	virtual void Heartbeat() = 0;
	//! Get information about our current organization.
	virtual void GetOrganization( Delegate<const Json::Value &> a_Callback ) = 0;
	//! Get organization admin list.
	virtual void GetOrgAdminList( Delegate<const Json::Value &> a_Callback ) = 0;
	//! Get current parent.
	virtual void GetParent( const std::string & a_ParentId, Delegate<const Json::Value &> a_Callback ) = 0;
	//! Get the current configuration data store in the gateway for the provided Robot Key and mac ID. The user
	//! is responsible for the ServieList object returned in the callback, they must call delete on the object
	//! when they are done.
	virtual void GetServices( Delegate<ServiceList *> a_Callback ) = 0;
};

struct SELF_API ServiceAttributes : public ISerializable
{
	RTTI_DECL();

	std::string		m_Key;
	std::string		m_Value;

	virtual void Serialize(Json::Value & json)
	{
		json[m_Key] = m_Value;
	}

	virtual void Deserialize(const Json::Value & json) 
	{
		for (Json::Value::iterator it = json.begin(); it != json.end(); ++it)
		{
			m_Key = it.key().asString();
			m_Value = (*it).asString();
		}
	}
};

struct SELF_API Service : public ISerializable
{
	RTTI_DECL();

	std::string             m_ServiceName;
	std::string		        m_Endpoint;
	std::string		        m_Username;
	std::string		        m_Password;
	std::string		        m_ServiceStatus;
	std::vector <ServiceAttributes>
		m_ServiceAttributes;

	//!ISerizliable interface
	virtual void Serialize(Json::Value & json)
	{
		json["serviceName"] = m_ServiceName;
		json["endpoint"] = m_Endpoint;
		json["username"] = m_Username;
		json["password"] = m_Password;
		json["serviceStatus"] = m_ServiceStatus;

		SerializeVector( "serviceAttributes", m_ServiceAttributes, json );
	}

	virtual void Deserialize(const Json::Value & json) 
	{
		m_ServiceName = json["serviceName"].asString();
		m_Endpoint = json["endpoint"].asString();
		m_Username = json["username"].asString();
		m_Password = json["password"].asString();
		m_ServiceStatus = json["serviceStatus"].asString();

		DeserializeVector( "serviceAttributes", json, m_ServiceAttributes );
	}
};

struct SELF_API ServiceList : public ISerializable
{
	RTTI_DECL();

	std::string             m_OrgId;
	std::string             m_GroupId;
	std::vector<Service>    m_Services;

	virtual void Serialize(Json::Value & json)
	{
		json["orgId"] = m_OrgId;
		json["groupId"] = m_GroupId;
		SerializeVector( "services", m_Services, json );
	}

	virtual void Deserialize(const Json::Value & json) 
	{
		m_OrgId = json["orgId"].asString();
		m_GroupId = json["groupId"].asString();
		DeserializeVector( "services", json, m_Services );
	}
};


#endif //SELF_IGATEWAY_H
