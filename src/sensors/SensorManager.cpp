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


#include "utils/Log.h"
#include "topics/TopicManager.h"
#include "SensorManager.h"
#include "SelfInstance.h"

#include "GazeData.h"
#include "MoodData.h"

#include "ISensor.h"
#include "AudioData.h"
#include "GazeData.h"
#include "HealthData.h"
#include "MoodData.h"
#include "RemoteDeviceData.h"
#include "SonarData.h"
#include "TextData.h"
#include "TouchData.h"
#include "VideoData.h"
#include "DepthVideoData.h"
#include "LaserData.h"
#include "ProxySensor.h"

RTTI_IMPL(IData, ISerializable);
RTTI_IMPL(AudioData, IData);
REG_SERIALIZABLE( AudioData );
RTTI_IMPL(GazeData, IData);
REG_SERIALIZABLE( GazeData );
RTTI_IMPL(HealthData, IData);
REG_SERIALIZABLE( HealthData );
RTTI_IMPL(MoodData, IData);
REG_SERIALIZABLE( MoodData );
RTTI_IMPL(RemoteDeviceData, IData);
REG_SERIALIZABLE( RemoteDeviceData );
RTTI_IMPL(SonarData, IData);
REG_SERIALIZABLE( SonarData );
RTTI_IMPL(TextData, IData);
REG_SERIALIZABLE( TextData );
RTTI_IMPL(TouchData, IData);
REG_SERIALIZABLE( TouchData );
RTTI_IMPL(VideoData, IData);
REG_SERIALIZABLE( VideoData );
RTTI_IMPL(DepthVideoData, IData);
REG_SERIALIZABLE( DepthVideoData );
RTTI_IMPL(LaserData, IData);
REG_SERIALIZABLE( LaserData );

SensorManager::SensorManager() : m_bActive( false ), m_pTopicManager( NULL )
{}

SensorManager::ISensorSP SensorManager::FindSensor( const std::string & a_SensorId ) const
{
	for(SensorList::const_iterator iSensor = m_Sensors.begin();
		iSensor != m_Sensors.end(); ++iSensor)
	{
		if ( (*iSensor)->GetSensorId() == a_SensorId )
			return *iSensor;
	}

	return ISensorSP();
}

bool SensorManager::FindSensorsByDataType(const std::string & a_DataType, SensorList & sensors) const
{
	for(SensorList::const_iterator iSensor = m_Sensors.begin();iSensor != m_Sensors.end(); ++iSensor)
	{
		if ( a_DataType.compare( (*iSensor)->GetDataType() ) == 0 )
			sensors.push_back( *iSensor );
	}

	return sensors.end() != sensors.begin();
}

bool SensorManager::Start()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;
	if (m_bActive)
		return false;

	m_bActive = true;

	m_pTopicManager = pInstance->GetTopicManager();
	assert( m_pTopicManager != NULL );

	// instantiate all sensor class types from the body.
	const SelfInstance::SensorList & sensors = pInstance->GetSensorList();
	for ( SelfInstance::SensorList::const_iterator iSensor = sensors.begin(); iSensor != sensors.end(); ++iSensor )
		AddSensor( (*iSensor) );

	m_pTopicManager->RegisterTopic( "sensor-manager", "application/json",
		DELEGATE( SensorManager, OnSubscriber, const ITopics::SubInfo &, this ) );
	m_pTopicManager->Subscribe( "sensor-manager", 
		DELEGATE( SensorManager, OnSensorEvent, const ITopics::Payload &, this ) );

	Log::Status( "SensorManager", "SensoryManager started." );
	return true;
}

bool SensorManager::Stop()
{
	if (!m_bActive)
		return false;
	
	if ( m_pTopicManager != NULL )
	{
		m_pTopicManager->Unsubscribe( "sensor-manager", this );
		m_pTopicManager->UnregisterTopic( "sensor-manager" );
	}

	Log::Status( "SensorManager", "SensorManager stopping." );
	m_Sensors.clear();

	m_bActive = false;
	return true;
}

bool SensorManager::AddSensor( const ISensorSP & a_spSensor, bool a_bOveride /*= false */)
{
	if ( !a_spSensor )
		return false;

	a_spSensor->SetSensorManager( this, a_bOveride );
	// set AFTER we call SetSensorManager() to prevent SetSensorManager() from finding it's own sensor on overrides
	m_Sensors.push_back( a_spSensor );			

	if ( a_spSensor->IsEnabled() )
	{
		if ( m_pTopicManager != NULL )
			a_spSensor->OnRegisterTopics( m_pTopicManager );

		RegistryMap::const_iterator iReg = m_RegistryMap.find( a_spSensor->GetDataType() );
		if ( iReg != m_RegistryMap.end() )
		{
			const RegistryList & list = iReg->second;
			for( RegistryList::const_iterator i = list.begin(); i != list.end(); ++i)
				(*i).m_OnAddSensor( a_spSensor.get() );
		}
	}

	Log::Status( "SensorManager", "Added sensor %s, Id: %s",
		a_spSensor->GetSensorName().c_str(), a_spSensor->GetSensorId().c_str() );

	return true;
}

bool SensorManager::RemoveSensor( const ISensorSP & a_spSensor )
{	
	for( SensorList::iterator iSensor = m_Sensors.begin(); iSensor != m_Sensors.end(); ++iSensor )
	{
		if ( (*iSensor) == a_spSensor )
		{
			RegistryMap::const_iterator iReg = m_RegistryMap.find( a_spSensor->GetDataType() );
			if ( iReg != m_RegistryMap.end() )
			{
				const RegistryList & list = iReg->second;
				for( RegistryList::const_iterator i = list.begin(); i != list.end(); ++i)
					(*i).m_OnRemoveSensor( a_spSensor.get() );
			}

			if ( m_pTopicManager != NULL )
				a_spSensor->OnUnregisterTopics( m_pTopicManager );
			if ( a_spSensor->IsActive() )
				a_spSensor->OnStop();

			a_spSensor->SetSensorManager( NULL, true );
			m_Sensors.erase( iSensor );
			return true;
		}
	}

	return false;
}

void SensorManager::RegisterForSensor( const std::string & a_DataType,
	OnSensor a_OnAddSensor, 
	OnSensor a_OnRemoveSensor )
{
	m_RegistryMap[ a_DataType ].push_back( SensorRegistry( a_OnAddSensor, a_OnRemoveSensor ) );

	for(SensorList::const_iterator iSensor = m_Sensors.begin();
		iSensor != m_Sensors.end(); ++iSensor)
	{
		if ( a_DataType.compare( (*iSensor)->GetDataType() ) == 0 )
			a_OnAddSensor( (*iSensor).get() );
	}
}

bool SensorManager::UnregisterForSensor( const std::string & a_DataType,
	void * a_pObject /*= NULL*/ )
{
	RegistryMap::iterator iReg = m_RegistryMap.find( a_DataType );
	if ( iReg != m_RegistryMap.end() )
	{
		bool bSuccess = false;
		if ( a_pObject != NULL )
		{
			RegistryList & list = iReg->second;
			for( RegistryList::iterator i = list.begin(); i != list.end(); )
			{
				if ( (*i).m_OnAddSensor.IsObject( a_pObject ) || (*i).m_OnRemoveSensor.IsObject( a_pObject ) )
				{
					// invoke the remove sensor callback before actually remove..
					for(SensorList::const_iterator iSensor = m_Sensors.begin();
						iSensor != m_Sensors.end(); ++iSensor)
					{
						if ( a_DataType.compare( (*iSensor)->GetDataType() ) == 0 )
							(*i).m_OnRemoveSensor( (*iSensor).get() );
					}

					list.erase( i++ );
					bSuccess = true;
				}
				else
					++i;
			}
		}
		else
		{
			m_RegistryMap.erase( iReg );
			bSuccess = true;
		}

		return bSuccess;
	}

	return false;
}


void SensorManager::PauseSensorType(const std::string & a_dataType)
{
	for(SensorList::iterator iSensor = m_Sensors.begin();
			iSensor != m_Sensors.end(); ++iSensor)
	{
		if( a_dataType.compare( (*iSensor)->GetDataType() ) == 0 )
			(*iSensor)->OnPause();
	}
}

void SensorManager::ResumeSensorType(const std::string & a_dataType)
{
	for(SensorList::iterator iSensor = m_Sensors.begin();
		iSensor != m_Sensors.end(); ++iSensor)
	{
		if( a_dataType.compare( (*iSensor)->GetDataType() ) == 0 )
			(*iSensor)->OnResume();
	}
}

void SensorManager::OnSensorOverride( ISensor * a_pSensor )
{
	assert( a_pSensor != NULL );
	assert( a_pSensor->IsOverridden() );

	Log::Status( "SensorManager", "Disabling sensor %s, Id: %s", 
		a_pSensor->GetSensorName().c_str(), a_pSensor->GetSensorId().c_str() );

	RegistryMap::const_iterator iReg = m_RegistryMap.find( a_pSensor->GetDataType() );
	if ( iReg != m_RegistryMap.end() )
	{
		const RegistryList & list = iReg->second;
		for( RegistryList::const_iterator i = list.begin(); i != list.end(); ++i)
			(*i).m_OnRemoveSensor( a_pSensor );
	}

	if ( m_pTopicManager != NULL )
		a_pSensor->OnUnregisterTopics( m_pTopicManager );
	if ( a_pSensor->IsActive() )
		a_pSensor->OnStop();
}

void SensorManager::OnSensorOverrideEnd( ISensor * a_pSensor )
{
	assert( a_pSensor != NULL );
	assert( !a_pSensor->IsOverridden() );

	Log::Status( "SensorManager", "Enabling sensor %s, Id: %s", 
		a_pSensor->GetSensorName().c_str(), a_pSensor->GetSensorId().c_str() );

	if ( m_pTopicManager != NULL )
		a_pSensor->OnRegisterTopics( m_pTopicManager );

	RegistryMap::const_iterator iReg = m_RegistryMap.find( a_pSensor->GetDataType() );
	if ( iReg != m_RegistryMap.end() )
	{
		const RegistryList & list = iReg->second;
		for( RegistryList::const_iterator i = list.begin(); i != list.end(); ++i)
			(*i).m_OnAddSensor( a_pSensor );
	}
}

void SensorManager::OnSubscriber( const ITopics::SubInfo & a_Info )
{
	if (! a_Info.m_Subscribed )
	{
		for(size_t i=0;i<m_Sensors.size();)
		{
			ProxySensor::SP spProxy = DynamicCast<ProxySensor>( m_Sensors[i] );
			++i;

			if (! spProxy )
				continue;

			if ( spProxy->GetOrigin() == a_Info.m_Origin )
			{
				Log::Status( "SensorManager", "Removing proxy sensor %s for origin %s", 
					spProxy->GetSensorId().c_str(), a_Info.m_Origin.c_str() );
				RemoveSensor( spProxy );

				i -= 1;		// back up one
			}
		}
	}
}

void SensorManager::OnSensorEvent( const ITopics::Payload & a_Payload )
{
	if ( a_Payload.m_RemoteOrigin[0] != 0 )
	{
		Json::Reader reader( Json::Features::strictMode() );

		Json::Value json;
		if ( reader.parse( a_Payload.m_Data, json ) )
		{
			if ( json["event"].isString() )
			{
				const std::string & ev = json["event"].asString();
				if ( ev == "add_sensor_proxy" )
				{
					const std::string & sensorId = json["sensorId"].asString();
					const std::string & sensorName = json["name"].asString();
					const std::string & dataType = json["data_type"].asString();
					const std::string & binaryType = json["binary_type"].asString();
					bool bOverride = json["override"].asBool();

					Log::Status( "SensorManager", "Adding proxy sensor %s, Id: %s, Override: %s", 
						sensorName.c_str(), sensorId.c_str(), bOverride ? "True" : "False" );

					ProxySensor::SP spProxy( new ProxySensor( sensorId, sensorName, dataType, binaryType, a_Payload.m_RemoteOrigin ) );
					AddSensor( spProxy, bOverride );
				}
				else if ( ev == "remove_sensor_proxy" )
				{
					bool bSuccess = false;

					const std::string & sensorId = json["sensorId"].asString();
					for( SensorList::iterator iSensor = m_Sensors.begin(); iSensor != m_Sensors.end(); ++iSensor )
					{
						if ( (*iSensor)->GetSensorId() == sensorId )
						{
							Log::Status( "SensorManager", "Removing proxy sensor %s, Id: %s", 
								(*iSensor)->GetSensorName().c_str(), sensorId.c_str() );
							bSuccess = RemoveSensor( *iSensor );
							break;
						}
					}

					if (! bSuccess )
						Log::Warning( "SensorManager", "Failed to remove proxy sensor %s", sensorId.c_str() );
				}
				else if ( ev == "error" )
				{
					const std::string & failed_event = json["failed_event"].asString();
					Log::Error( "SensorManager", "Received error on event: %s", failed_event.c_str() );
				}
				else
				{
					Log::Warning( "SensorManager", "Received unknown event: %s", json.toStyledString().c_str() );
				}
			}
		}
		else
			Log::Error( "SensorManager", "Failed to parse json: %s", reader.getFormattedErrorMessages().c_str() );
	}
}
