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


#include "ProxySensor.h"
#include "SelfInstance.h"
#include "topics/ITopics.h"

RTTI_IMPL( ProxySensor, ISensor );
REG_SERIALIZABLE( ProxySensor );

ProxySensor::ProxySensor( const std::string & a_SensorId,
	const std::string & a_SensorName, 
	const std::string & a_DataType,
	const std::string & a_BinaryType,
	const std::string & a_Origin ) : 
	ISensor( a_SensorName, a_SensorId ),
	m_DataType( a_DataType ),
	m_BinaryType( a_BinaryType ),
	m_Origin( a_Origin )
{}

ProxySensor::ProxySensor() : ISensor("Proxy")
{}

ProxySensor::~ProxySensor()
{}

void ProxySensor::Serialize(Json::Value & json)
{
	ISensor::Serialize( json );

	json["m_DataType"] = m_DataType;
	json["m_BinaryType"] = m_BinaryType;
	json["m_Origin"] = m_Origin;
}

void ProxySensor::Deserialize(const Json::Value & json)
{
	ISensor::Deserialize( json );

	m_DataType = json["m_DataType"].asString();
	m_BinaryType = json["m_BinaryType"].asString();
	m_Origin = json["m_Origin"].asString();
}

bool ProxySensor::OnStart()
{
	SendEvent( "start_sensor" );
	return true;
}

bool ProxySensor::OnStop()
{
	SendEvent( "stop_sensor" );
	return true;
}

void ProxySensor::OnPause()
{
	SendEvent( "pause_sensor" );
}

void ProxySensor::OnResume()
{
	SendEvent( "resume_sensor" );
}

void ProxySensor::OnRegisterTopics( ITopics * a_pTopics )
{
	ISensor::OnRegisterTopics( a_pTopics );

	m_ProxyTopicId = "sensor-proxy-";
	m_ProxyTopicId += GetGUID();

	a_pTopics->RegisterTopic( m_ProxyTopicId, m_BinaryType );
	a_pTopics->Subscribe( m_ProxyTopicId,
		DELEGATE( ProxySensor, OnSensorData, const ITopics::Payload &, this ) );
}

void ProxySensor::OnUnregisterTopics( ITopics * a_pTopics  )
{
	ISensor::OnUnregisterTopics( a_pTopics );

	a_pTopics->Unsubscribe( m_ProxyTopicId, this );
	a_pTopics->UnregisterTopic( m_ProxyTopicId );
}

void ProxySensor::SendEvent( const std::string & a_EventName )
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );
	ITopics * pTopics = pInstance->GetTopics();
	assert( pTopics != NULL );

	Json::Value ev;
	ev["event"] = a_EventName;
	ev["sensorId"]  = GetGUID();

	pTopics->Send( m_Origin, "sensor-manager", ev.toStyledString() );
}

void ProxySensor::OnSensorData( const ITopics::Payload & a_Payload )
{
	bool bSuccess = false;

	ISerializable * pUncasted = ISerializable::GetSerializableFactory().CreateObject( m_DataType );
	if ( pUncasted != NULL )
	{
		IData * pData = DynamicCast<IData>( pUncasted );
		if ( pData != NULL )
		{
			if ( pData->FromBinary( a_Payload.m_Type, a_Payload.m_Data ) )
			{
				SendData( pData );
				bSuccess = true;
			}
		}
	}

	if (! bSuccess )
	{
		Log::Error( "ProxySensor", "Failed to parse binary data for %s", m_DataType.c_str() );
		delete pUncasted;
	}
}
