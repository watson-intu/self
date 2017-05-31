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


#ifndef SELF_ITHING_H
#define SELF_ITHING_H

#include <string>
#include <vector>
#include <stdio.h>

#include "boost/enable_shared_from_this.hpp"
#include "boost/shared_ptr.hpp"

#include "models/IGraph.h"
#include "utils/UniqueID.h"
#include "utils/Time.h"
#include "utils/ISerializable.h"
#include "utils/RTTI.h"
#include "utils/ParamsMap.h"
#include "utils/Delegate.h"
#include "utils/TimerPool.h"

#include "SelfLib.h"		// include last always

class BlackBoard;
class ThingEvent;
class Proxy;

//! These enums are setup as bit flags so the subscribe can subscribe for specific events.
enum ThingEventType
{
	TE_NONE			= 0x0,			// no flags
	TE_ADDED		= 0x1,			// IThing has been added
	TE_REMOVED		= 0x2,			// IThing has been removed
	TE_STATE		= 0x4,			// state of IThing has changed.
	TE_IMPORTANCE	= 0x8,			// Importance of IThing has changed.
	TE_GUID			= 0x10,			// GUID has been changed
	TE_DATA			= 0X20,			// data of this thing has been changed

	TE_ALL = TE_ADDED | TE_REMOVED | TE_STATE | TE_IMPORTANCE | TE_GUID | TE_DATA,
    TE_ADDED_OR_STATE = TE_ADDED | TE_STATE
};

//! All things fall into one of the following categories, things of different types will not 
//! be directly attached to each other but a proxy will be created and connected via GUID
enum ThingCategory
{
	TT_INVALID = -1,
	TT_PERCEPTION,
	TT_COGNITIVE,
	TT_MODEL
};

//! This abstract interface is the base class for any object that can be added
//! to the blackboard. The idea is that things connect to each other automatically
//! using a trained graph, then drive the goals of the system. 
//! These things should automatically remove them-selves after they are no longer needed.

class SELF_API IThing : public ISerializable, public boost::enable_shared_from_this<IThing>
{
public:
	RTTI_DECL();

	static const std::string EMPTY_STRING;

	//! Types
	typedef boost::shared_ptr<IThing>	SP;
	typedef boost::weak_ptr<IThing>		WP;
	typedef std::vector< SP >			ThingList;

	//! Construction
	IThing( ThingCategory a_eCategory, 
		const std::string & a_DataType = "", 
		const Json::Value & a_Data = Json::Value::nullRef,
		float a_LifeSpan = 3600.0f,
		float a_fImportance = 1.0f,
		const std::string & a_State = "ADDED" ) :
		m_pBlackBoard( NULL ),
		m_pParent( NULL ),
		m_eCategory( a_eCategory ),
		m_DataType( a_DataType ),
		m_Data( a_Data ),
		m_State(a_State),
		m_fImportance( a_fImportance ),
		m_fLifeSpan(a_LifeSpan)
	{
		NewGUID();
	}
	IThing( ThingCategory a_eCategory, 
		float a_LifeSpan,
		const std::string & a_State = "ADDED" ) :
		m_pBlackBoard( NULL ),
		m_pParent( NULL ),
		m_eCategory( a_eCategory ),
		m_State(a_State),
		m_fImportance( 1.0f ),
		m_fLifeSpan(a_LifeSpan)
	{
		NewGUID();
	}
	IThing() : 
		m_pBlackBoard( NULL ), 
		m_pParent( NULL ),
		m_eCategory(TT_INVALID),
		m_State("ADDED"),
		m_fImportance( 1.0f ),
		m_fLifeSpan(0.0f)
	{
		NewGUID();
	}
	virtual ~IThing()
	{
		for(size_t i=0;i<m_Children.size();++i)
			m_Children[i]->m_pParent = NULL;
	}

	//! IWIdget interface
	virtual void SetGUID(const std::string & a_GUID );
	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Get our owning blackboard object.
	BlackBoard * GetBlackBoard() const
	{
		return m_pBlackBoard;
	}
	//! Get the category for this object
	ThingCategory GetCategory() const
	{
		return m_eCategory;
	}
	//! Get the previous GUID
	const std::string & GetPrevGUID() const
	{
		return m_PrevGUID;
	}
	//! Get the ID of the proxy object that points at this object if any
	const std::string & GetProxyID() const
	{
		return m_ProxyID;
	}
	//! Get the creation time for this thing
	double GetCreateTime() const
	{
		return m_CreateTime.GetEpochTime();
	}
	//! Age of this thing in seconds
	double GetAge() const
	{
		return Time().GetEpochTime() - m_CreateTime.GetEpochTime();
	}
	float GetLifeSpan() const
	{
		return m_fLifeSpan;
	}

	//! The importance of this thing
	float GetImportance() const
	{
		return m_fImportance;
	}

	//! Get the current state of this thing.
	const std::string & GetState() const
	{
		return m_State;
	}

	//! Returns true if we have a parent object.
	bool HasParent() const
	{
		return m_pParent != NULL;
	}

	//! Returns our parent object.
	SP GetParent( bool a_bEnumProxies = false ) const
	{
		if ( a_bEnumProxies && m_ProxyID.size() > 0 && m_pBlackBoard != NULL )
			return GetProxy();
		if ( m_pParent != NULL )
			return m_pParent->shared_from_this();
		return SP();
	}
	const std::string & GetParentGUID( bool a_bEnumProxies = false ) const
	{
		SP spParent = GetParent( a_bEnumProxies );
		if ( spParent )
			return spParent->GetGUID();
		return EMPTY_STRING;
	}

	//! Other things this thing is related too
	const ThingList & GetChildren() const
	{
		return m_Children;
	}

	//! Returns true if this object is a parent of this object.
	bool IsParent( const IThing::SP & a_spThing, bool a_bEnumProxies = false ) const
	{
		SP spParent = GetParent( a_bEnumProxies );
		while( spParent )
		{
			if ( spParent == a_spThing )
				return true;
			spParent = spParent->GetParent( a_bEnumProxies );
		}
		return false;
	}

	//! This finds a parent object of the given type.
	template<typename T>
	boost::shared_ptr<T> FindParentType( bool a_bEnumProxies = false ) const 
	{
		SP spParent = GetParent( a_bEnumProxies );
		while (spParent)
		{
			boost::shared_ptr<T> spCasted = DynamicCast<T>(spParent);
			if (spCasted)
				return spCasted;
			spParent = spParent->GetParent( a_bEnumProxies );
		}

		return boost::shared_ptr<T>();
	}

	//! Find all children of the given type
	template<typename T>
	bool FindChildrenType( std::vector< boost::shared_ptr<T> > & a_Children, bool a_bRecursive = true ) const
	{
		for(size_t i=0;i<m_Children.size();++i)
		{
			const SP & spChild = m_Children[i];
			if ( a_bRecursive )
				spChild->FindChildrenType<T>( a_Children, true );

			boost::shared_ptr<T> spCasted = DynamicCast<T>( spChild );
			if ( spCasted )
				a_Children.push_back( spCasted );
		}

		return a_Children.size() > 0;
	}

	SP FindChildType( const std::string & a_DataType, bool a_bRecursive = true ) const
	{
		for(size_t i=0;i<m_Children.size();++i)
		{
			const SP & spChild = m_Children[i];
			if ( spChild->GetDataType() == a_DataType || spChild->GetRTTI().GetName() == a_DataType )
				return spChild;

			if ( a_bRecursive )
			{
				SP spFound = spChild->FindChildType( a_DataType );
				if ( spFound )
					return spFound;
			}
		}

		return SP();
	}

	//! Get the object type
	const std::string & GetDataType() const
	{
		return m_DataType;
	}
	const Json::Value & GetData() const
	{
		return m_Data;
	}
	const IVertex::SP & GetVertex() const
	{
		return m_spVertex;
	}

	//! Mutators
	void SetBlackBoard(BlackBoard * a_pBlackBoard);

	void SetCategory( ThingCategory a_eCategory )
	{
		m_eCategory = a_eCategory;
	}
	void SetProxyID( const std::string & a_ProxyID )
	{
		m_ProxyID = a_ProxyID;
	}

	void SetImportance(float a_fImportance)
	{
		if ( a_fImportance != m_fImportance )
		{
			m_fImportance = a_fImportance;
			OnImportanceChanged();
		}
	}

	void SetState( const std::string & a_NewState )
	{
		if ( m_State != a_NewState )
		{
			m_State = a_NewState;
			OnStateChanged();
		}
	}

	void ResetCreateTime();
	void AddChild(const SP & a_spThing);
	bool RemoveChild(size_t index);
	bool RemoveChild(IThing * a_pThing);
	void RemoveAllChildren();
	bool RemoveThis();

	//! subscribe to this specific instance for notifications.
	void Subscribe(Delegate<const ThingEvent &> a_Subscriber);
	bool Unsubscribe(void * a_pObject);

	//! Sends the given event to all subscribers and the owning blackboard.
	void SendEvent(const ThingEvent & a_Event);

	void SetDataType( const std::string & a_DataType )
	{
		m_DataType = a_DataType;
	}
	void SetData( const Json::Value & a_Data )
	{
		m_Data = a_Data;
		OnDataChanged();
	}
	Json::Value & GetData() 
	{
		return m_Data;
	}
	void SetVertex( const IVertex::SP & a_spModel )
	{
		m_spVertex = a_spModel;
	}

	//! Operator[] for accessing contained data
	Json::Value & operator[]( const std::string & a_Key )
	{
		return m_Data[ a_Key ];
	}
	const Json::Value & operator[]( const std::string & a_Key ) const
	{
		return m_Data[ a_Key ];
	}

	//! Interface
	virtual void OnAttached();				// this is invoked AFTER we have been attached into the hierarchy
	virtual void OnDetach();				// this is invoked BEOFRE we are detached from the hierarchy.
	virtual void OnImportanceChanged();		// invoked when the importance is changed
	virtual void OnStateChanged();			// invoked when the state is changed
	virtual void OnGuidChanged();			// invoked when the GUID is changed
	virtual void OnDataChanged();			// invoked when the data is changed

	virtual void OnLifeSpanExpired();		// invoked by the timer when the lifespan of this thing is up

	//! Sort the things by time, ascending order places the newest thing last in the last.
	static void SortChron( ThingList & a_Things, bool a_bAcending = true );
	//! Returns a text description of the event type.
	static const char * ThingEventTypeText( ThingEventType a_eType );
	//! Helper function to make a Json::Object from a key/value pair.
	static Json::Value JsonObject( const char * a_pKey, const Json::Value & a_Value );

protected:

	//! Types
	typedef Delegate<const ThingEvent &>	Subscriber;
	typedef std::list< Subscriber >			SubscriberList;

	//! Data
	BlackBoard *		m_pBlackBoard;		// blackboard containing this thing
	ThingCategory		m_eCategory;		// category of blackboard object
	std::string			m_PrevGUID;			// previous GUID
	std::string			m_ProxyID;			// our proxy ID
	Time				m_CreateTime;		// when was this thing made in seconds since epoch
	float				m_fImportance;		// how important is this thing
	std::string			m_State;			// state of this object
	IThing *			m_pParent;			// our parent thing
	ThingList			m_Children;			// things created from this thing

	std::string			m_DataType;			// type of data contained by this object
	Json::Value			m_Data;				// data of this object

	IVertex::SP			m_spVertex;			// vertex in models of the world, self, or others for this object. 
											// this holds all persisted data for this object.

	float				m_fLifeSpan;		// life-span of this thing in seconds
	TimerPool::ITimer::SP
						m_spDetachTimer;	// timer that will remove this object after it's life is over
	SubscriberList		m_Subscribers;		// subscribers for this instance

	SP					GetProxy() const;
	void				StartDetachTimer();
};

//-----------------------------------------------

class ThingEvent
{
public:
	ThingEvent(IThing::SP a_pThing, ThingEventType a_Event) : m_spThing(a_pThing), m_Event(a_Event)
	{}
	ThingEvent() : m_Event(TE_ADDED)
	{}

	ThingEventType GetThingEventType() const
	{
		return m_Event;
	}

	const IThing::SP & GetIThing() const
	{
		return m_spThing;
	}

private:
	IThing::SP		m_spThing;
	ThingEventType 	m_Event;
};

#endif
