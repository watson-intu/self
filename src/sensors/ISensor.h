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


#ifndef ISENSOR_H
#define ISENSOR_H

#include <string>
#include <map>
#include <stdio.h>

#include "boost/enable_shared_from_this.hpp"
#include "boost/shared_ptr.hpp"

#include "topics/ITopics.h"
#include "utils/ISerializable.h"
#include "utils/Delegate.h"
#include "utils/RTTI.h"
#include "utils/StringUtil.h"
#include "SensorManager.h"
#include "IData.h"
#include "SelfLib.h"
#undef GetBinaryType

class SensorManager;

//! This is the base class for all sensors that can receive some type of input from an external 
//! source. Examples include connected video camera, microphone input, or data through a connected
//! socket.
class SELF_API ISensor : public ISerializable, 
	public boost::enable_shared_from_this<ISensor>
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<ISensor>	SP;

	//! Construction
	ISensor( const std::string & a_SensorName );
	ISensor( const std::string & a_SensorName, const std::string & a_SensorId );
	virtual ~ISensor();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Interface
	virtual const char * GetDataType() = 0;
	virtual const char * GetBinaryType() { return "application/json"; }

	//! This is invoked when the first subscriber subscribes to this sensor
	virtual bool OnStart() = 0;
	//! This is invoked when the last subscriber un-subscribes.
	virtual bool OnStop() = 0;
	//! This is invoked to pause this sensor.
	virtual void OnPause() = 0;
	//! This is invoked to restart this sensor.
	virtual void OnResume() = 0;

	//! This is invoke by the SensorManager to register any topics provided by this sensor.
	virtual void OnRegisterTopics( ITopics * a_pTopics );
	//! This is invoked to de-register this sensor from the topic system
	virtual void OnUnregisterTopics( ITopics * a_pTopics );

	//! Accessors
	bool IsEnabled() const
	{
		return m_bEnabled;
	}
	bool IsActive() const 
	{
		return m_Subscribers.size() > 0 || m_TopicSubscribers > 0;
	}
	const std::string & GetSensorName()
	{
		return m_SensorName;
	}
	const std::string & GetSensorId() const
	{
		return GetGUID();
	}
	bool IsOverridden() const
	{
		return m_Overrides > 0;
	}
	bool IsPaused() const
	{
		return m_Paused > 0;
	}

	//! Set the friendly name for this sensor.
	void SetSensorName( const std::string & a_Name );
	//! Set the sensor manager and specify if this sensor will override any sensors of the same data type
	void SetSensorManager( SensorManager * a_pManager, bool a_bOverride );
	//! Subscribe to this sensor, any incoming data should be sent to the given delegate. 
	void Subscribe( Delegate<IData *> a_Subscriber );
	//! Un-subscribe from this sensor, you pass the this pointer of the object that the delegate was initialized with..
	bool Unsubscribe( void * obj );

	void AddOverride();
	void RemoveOverride();

protected:

	//! Types
	typedef std::list< Delegate<IData *> > SubcriberList;

	//! Data
	bool				m_bEnabled;
	std::string			m_SensorName;			//!< Friendly name for this sensor
	SubcriberList		m_Subscribers;			//!< List of subscribers
	ITopics *			m_pTopics;				//!< ITopic interface pointer
	std::string			m_TopicId;				//!< TopicId for this sensor
	int					m_TopicSubscribers;
	int					m_Overrides;
	volatile int		m_Paused;

	SensorManager *		m_pManager;
	std::vector< SP >	m_Overriden;

	void OnSubscriber( const ITopics::SubInfo & a_Sub );
	//! Send data to all subscribers of this sensor, note this function takes ownership
	//! of the IData object and will delete it once it's done processing with all sensors.
	void SendData( IData * a_Data );
};

inline void ISensor::SetSensorName( const std::string & a_Name )
{
	m_SensorName = a_Name;
}

#endif
