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


#ifndef SELF_PROXY_SENSOR_H
#define SELF_PROXY_SENSOR_H

#include "sensors/ISensor.h"
#include "SelfLib.h"

//! This sensor class is used to represent a remote sensor that is running outside this self instance
class SELF_API ProxySensor : public ISensor
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr< ProxySensor >		SP;
	typedef boost::weak_ptr< ProxySensor >			WP;

	//! Construction
	ProxySensor( const std::string & a_SensorId,
		const std::string & a_SensorName, 
		const std::string & a_DataType,
		const std::string & a_BinaryType,
		const std::string & a_Origin );
	ProxySensor();
	~ProxySensor();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! ISensor interface
	virtual const char * GetDataType()
	{
		return m_DataType.c_str();
	}
	virtual const char * GetBinaryType()
	{
		return m_BinaryType.c_str();
	}

	virtual bool OnStart();
	virtual bool OnStop();
	virtual void OnPause();
	virtual void OnResume();
	virtual void OnRegisterTopics( ITopics * a_pTopics );
	virtual void OnUnregisterTopics( ITopics * a_pTopics  );

	const std::string & GetOrigin() const
	{
		return m_Origin;
	}

protected:
	//! Data
	std::string m_DataType;
	std::string m_BinaryType;
	std::string m_Origin;

	std::string m_ProxyTopicId;

	void SendEvent( const std::string & a_EventName );
	void OnSensorData( const ITopics::Payload & a_Payload );
};

#endif
