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


#include "IThing.h"
#include "BlackBoard.h"
#include "Proxy.h"
#include "SelfInstance.h"

#include <stdio.h>
#include <algorithm>

RTTI_IMPL(IThing, ISerializable);
REG_SERIALIZABLE( IThing );

const std::string IThing::EMPTY_STRING;

void IThing::SetGUID(const std::string & a_GUID )
{
	m_PrevGUID = GetGUID();
	IWidget::SetGUID( a_GUID );

	if ( m_pBlackBoard != NULL )
		OnGuidChanged();
}

void IThing::Serialize(Json::Value & json)
{
	json["m_eCategory"] = (int)m_eCategory;
	if ( !m_ProxyID.empty() )
		json["m_ProxyID"] = m_ProxyID;
	json["m_CreateTime"] = m_CreateTime.GetEpochTime();
	json["m_fImportance"] = m_fImportance;
	json["m_State"] = m_State;
	json["m_fLifeSpan"] = m_fLifeSpan;

	if ( m_DataType.size() > 0 )
		json["m_DataType"] = m_DataType;
	if (! m_Data.isNull() )
		json["m_Data"] = m_Data;

	int index = 0;
	for( ThingList::iterator iChild = m_Children.begin(); iChild != m_Children.end(); ++iChild )
		json["m_Children"][index++] = ISerializable::SerializeObject( (*iChild).get() );
}

void IThing::Deserialize(const Json::Value & json)
{
	if ( json["m_eCategory"].isInt() )
		m_eCategory = (ThingCategory)json["m_eCategory"].asInt();
	if ( json["m_ProxyID"].isString() )
		m_ProxyID = json["m_ProxyID"].asString();
	if ( json["m_CreateTime"].isDouble() )
		m_CreateTime = Time( json["m_CreateTime"].asDouble() );
	if ( json["m_fImportance"].isDouble() )
		m_fImportance = json["m_fImportance"].asFloat();
	if ( json["m_State"].isString() )
		m_State = json["m_State"].asString();
	if ( json["m_fLifeSpan"].isDouble() )
		m_fLifeSpan = json["m_fLifeSpan"].asFloat();

	if ( json["m_DataType"].isString() )
		m_DataType = json["m_DataType"].asString();
	if ( json["m_Data"].isObject() )
		m_Data = json["m_Data"];

	RemoveAllChildren();

	const Json::Value & children = json["m_Children"];
	for( Json::ValueConstIterator iChild = children.begin(); iChild != children.end(); ++iChild )
		AddChild( SP( DynamicCast<IThing>( ISerializable::DeserializeObject( *iChild ) ) ) );
}

void IThing::ResetCreateTime()
{
	m_CreateTime = Time();
	StartDetachTimer();
}

void IThing::AddChild( const SP & a_spThing )
{
	if ( a_spThing )
	{
		if ( a_spThing->GetCategory() == GetCategory() )
		{
			// check if we are already attached, if so then detach from our previous parent automatically..
			if ( a_spThing->GetBlackBoard() != NULL )
				a_spThing->RemoveThis();

			m_Children.push_back( a_spThing );
			a_spThing->m_pParent = this;

			//Log::Debug("IThing", "Adding a child to the parent with the parent being: %s", GetGUID().c_str());
			a_spThing->SetBlackBoard( m_pBlackBoard );		// this will invoke OnAttach()
		}
		else  // if the type if different, attach a proxy instead and 
		{
			Proxy::SP spProxy( new Proxy( GetCategory(), a_spThing ) );

			m_Children.push_back( spProxy );
			spProxy->m_pParent = this;
			spProxy->SetBlackBoard( m_pBlackBoard );

			// add the thing to the correct root object
			SelfInstance::GetInstance()->GetBlackBoard()->AddThing( a_spThing );
		}
	}
}

bool IThing::RemoveChild(size_t index)
{
	if ( index < m_Children.size() )
	{
		IThing * pChild = m_Children[ index ].get();
		pChild->SetBlackBoard( NULL );
		pChild->m_pParent = NULL;

		m_Children.erase( m_Children.begin() + index );
		return true;
	}

	return false;
}

bool IThing::RemoveChild( IThing * a_pThing )
{
	for(size_t i=0;i<m_Children.size();++i)
	{
		if ( m_Children[i].get() == a_pThing )
		{
			a_pThing->SetBlackBoard( NULL );		// will call OnDetach()
			a_pThing->m_pParent = NULL;

			m_Children.erase( m_Children.begin() + i );
			return true;
		}
	}
	return false;
}

void IThing::RemoveAllChildren()
{
	for(size_t i=0;i<m_Children.size();++i)
	{
		IThing * pChild = m_Children[i].get();
		pChild->SetBlackBoard( NULL );
		pChild->m_pParent = NULL;
	}

	m_Children.clear();
}


bool IThing::RemoveThis()
{
	if ( m_pParent != NULL )
		return m_pParent->RemoveChild( this );

	return false;
}

void IThing::Subscribe(Delegate<const ThingEvent &> a_Subscriber)
{
	m_Subscribers.push_back(a_Subscriber);
}

bool IThing::Unsubscribe(void * a_pObject)
{
	for (SubscriberList::iterator iSub = m_Subscribers.begin(); 
		iSub != m_Subscribers.end(); ++iSub)
	{
		if ((*iSub).IsObject(a_pObject))
		{
			m_Subscribers.erase(iSub);
			return true;
		}
	}

	return false;
}

void IThing::SendEvent(const ThingEvent & a_Event)
{
	if (m_pBlackBoard != NULL)
		m_pBlackBoard->OnThingEvent(a_Event);

	for (SubscriberList::iterator iSub = m_Subscribers.begin();
		iSub != m_Subscribers.end(); )
	{
		SubscriberList::iterator iInvoke = iSub++;
		if ((*iInvoke).IsValid())
			(*iInvoke)(a_Event);
	}
}

void IThing::SetBlackBoard(BlackBoard * a_pBlackBoard)
{
	if ( m_pBlackBoard != a_pBlackBoard )
	{
		if ( a_pBlackBoard == NULL )
		{
			// NOTE we invoke the OnDetach() before we clear the pointer, just in case the object needs to update the blackboard!
			OnDetach();		
			m_pBlackBoard = a_pBlackBoard;
			m_spDetachTimer.reset();
		}
		else
		{
			m_pBlackBoard = a_pBlackBoard;
			StartDetachTimer();
			OnAttached();
		}

		for(size_t i=0;i<m_Children.size();++i)
			m_Children[i]->SetBlackBoard( a_pBlackBoard );

	}
}

void IThing::OnAttached()
{
	SendEvent( ThingEvent( shared_from_this(), TE_ADDED) );
}

void IThing::OnDetach()
{
	SendEvent(ThingEvent(shared_from_this(), TE_REMOVED));
}

void IThing::OnImportanceChanged()
{
	SendEvent(ThingEvent(shared_from_this(), TE_IMPORTANCE));
}

void IThing::OnStateChanged()
{
	SendEvent(ThingEvent(shared_from_this(), TE_STATE));
}

void IThing::OnGuidChanged()
{
	SendEvent(ThingEvent(shared_from_this(), TE_GUID));
}

void IThing::OnDataChanged()
{
	SendEvent(ThingEvent(shared_from_this(), TE_DATA));
}

void IThing::OnLifeSpanExpired()
{
	m_spDetachTimer.reset();
	RemoveThis();
}

static bool SortAscending( const IThing::SP & a1, const IThing::SP & a2 )
{
	return a1->GetCreateTime() < a2->GetCreateTime();
}

static bool SortDecending( const IThing::SP & a1, const IThing::SP & a2 )
{
	return a1->GetCreateTime() > a2->GetCreateTime();
}

void IThing::SortChron( ThingList & a_Things, bool a_bAcending /*= true*/ )
{
	std::sort( a_Things.begin(), a_Things.end(), a_bAcending ? SortAscending : SortDecending );
}

const char * IThing::ThingEventTypeText( ThingEventType a_eType )
{
	if ( a_eType == TE_ADDED )
		return "add_object";
	else if ( a_eType == TE_REMOVED )
		return "remove_object";
	else if ( a_eType == TE_STATE )
		return "set_object_state";
	else if ( a_eType == TE_IMPORTANCE )
		return "set_object_importance";
	else if ( a_eType == TE_GUID )
		return "guid_changed";
	else if ( a_eType == TE_DATA )
		return "data_changed";

	return "UNKNOWN";
}

Json::Value IThing::JsonObject( const char * a_pKey, const Json::Value & a_Value )
{
	Json::Value obj;
	obj[a_pKey] = a_Value;

	return obj;
}

IThing::SP IThing::GetProxy() const
{
	if ( m_pBlackBoard != NULL )
		return m_pBlackBoard->FindThing( m_ProxyID );
	return SP();
}

void IThing::StartDetachTimer()
{
	TimerPool * pTimerPool = TimerPool::Instance();
	if (pTimerPool != NULL && m_fLifeSpan > 0.0f )
	{
		float fLifeRemaining = (float)((m_CreateTime.GetEpochTime() + m_fLifeSpan) - Time().GetEpochTime());
		m_spDetachTimer = pTimerPool->StartTimer(VOID_DELEGATE(IThing, OnLifeSpanExpired, shared_from_this()), fLifeRemaining, true, false);
	}
	else
		m_spDetachTimer.reset();
}

