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


#include "ISensor.h"
#include "utils/UniqueID.h"

RTTI_IMPL( ISensor, ISerializable );

ISensor::ISensor( const std::string & a_SensorName ) : 
	m_bEnabled( true ),
	m_SensorName( a_SensorName ),
	m_pTopics( NULL ),
	m_TopicSubscribers( 0 ),
	m_Overrides( 0 ),
	m_Paused( 0 ),
	m_pManager( NULL )
{
	NewGUID();
}

ISensor::ISensor( const std::string & a_SensorName, const std::string & a_SensorId ) :
	m_bEnabled( true ),
	m_SensorName( a_SensorName ),
	m_pTopics( NULL ),
	m_TopicSubscribers( 0 ),
	m_Overrides( 0 ),
	m_Paused( 0 ),
	m_pManager( NULL )
{
	SetGUID( a_SensorId );
}

ISensor::~ISensor()
{}

void ISensor::Serialize(Json::Value & json)
{
	json["m_bEnabled"] = m_bEnabled;
	json["m_SensorName"] = m_SensorName;
}

void ISensor::Deserialize(const Json::Value & json)
{
	if ( json["m_bEnabled"].isBool() )
		m_bEnabled = json["m_bEnabled"].asBool();
	if ( json["m_SensorName"].isString() )
		m_SensorName = json["m_SensorName"].asString();
}

void ISensor::OnRegisterTopics( ITopics * a_pTopics )
{
	assert( m_pTopics == NULL );
	m_pTopics = a_pTopics;
	m_TopicId = StringUtil::Format( "sensor-%s", m_SensorName.c_str() );
	m_pTopics->RegisterTopic( m_TopicId, GetBinaryType(), 
		DELEGATE( ISensor, OnSubscriber, const ITopics::SubInfo &, this ) );
}

//! This is invoked to de-register this sensor from the topic system
void ISensor::OnUnregisterTopics( ITopics * a_pTopics )
{
	if ( m_pTopics == a_pTopics )
	{
		assert( m_pTopics == a_pTopics );
		a_pTopics->UnregisterTopic( m_TopicId );
		m_pTopics = NULL;
	}
}

void ISensor::SetSensorManager( SensorManager * a_pManager, bool a_bOverride )
{
	m_pManager = a_pManager;
	if ( m_bEnabled && a_bOverride )
	{
		if ( a_pManager != NULL )
		{
			assert( m_Overriden.size() == 0 );
			if ( a_pManager->FindSensorsByDataType( GetDataType(), m_Overriden ) )
			{
				for(size_t i=0;i<m_Overriden.size();++i)
					m_Overriden[i]->AddOverride();
			}
		}
		else if ( m_Overriden.size() > 0 )
		{
			for(size_t i=0;i<m_Overriden.size();++i)
				m_Overriden[i]->RemoveOverride();
			m_Overriden.clear();
		}
	}
}

//! Subscribe to this sensor, any incoming data should be sent to the given delegate. 
void ISensor::Subscribe( Delegate<IData *> a_Subscriber )
{
	bool bStart = (m_Subscribers.size() + m_TopicSubscribers) == 0 && !IsOverridden();
	m_Subscribers.push_back(a_Subscriber);

	if ( m_bEnabled && bStart )
		OnStart();
}

//! Un-subscribe from this sensor, you pass the this pointer of the object that the delegate was initialized with..
bool ISensor::Unsubscribe( void * obj )
{
	for (SubcriberList::iterator iSub = m_Subscribers.begin();iSub != m_Subscribers.end(); )
	{
		if ((*iSub).IsObject(obj))
			m_Subscribers.erase(iSub++);
		else
			++iSub;
	}

	if ( m_bEnabled && (m_Subscribers.size() + m_TopicSubscribers) == 0 )
		OnStop();

	return true;
}

void ISensor::AddOverride()
{
	assert( m_Overrides >= 0 );
	bool bDisableSensor = m_Overrides == 0;
	m_Overrides += 1;
	if ( bDisableSensor && m_pManager != NULL )
		m_pManager->OnSensorOverride( this );
}

void ISensor::RemoveOverride( )
{
	assert( m_Overrides > 0 );
	m_Overrides -= 1;
	if ( m_Overrides == 0 && m_pManager != NULL )
		m_pManager->OnSensorOverrideEnd( this );
}

void ISensor::OnSubscriber( const ITopics::SubInfo & a_Sub )
{
	if ( a_Sub.m_Subscribed )
	{
		bool bStart = (m_Subscribers.size() + m_TopicSubscribers) == 0;
		m_TopicSubscribers += 1;

		if ( m_bEnabled && bStart )
			OnStart();
	}
	else
	{
		m_TopicSubscribers -= 1;

		if ( m_bEnabled && (m_Subscribers.size() + m_TopicSubscribers) == 0 )
			OnStop();
	}
}

//! Send data to all subscribers of this sensor, note this function takes ownership
//! of the IData object and will delete it once it's done processing with all sensors.
void ISensor::SendData( IData * a_Data )
{
	a_Data->SetOrigin( this );

	if ( m_TopicSubscribers > 0 )
	{
		std::string data;
		if (a_Data->ToBinary(data))
			m_pTopics->Publish(m_TopicId, data, false, true );
	}

	if ( m_Subscribers.begin() != m_Subscribers.end() )
	{
		for(SubcriberList::iterator iSub = m_Subscribers.begin();
		iSub != m_Subscribers.end(); ++iSub )
		{
			(*iSub)( a_Data );
		}
	}

	// TODO: more than likely, we may want to use a shared pointer or reference counting system
	// so subscribers can hang onto the data for a while.
	delete a_Data;
	a_Data = NULL;
}
