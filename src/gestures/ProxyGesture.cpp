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


#include "ProxyGesture.h"
#include "SelfInstance.h"
#include "GestureManager.h"
#include "utils/UniqueID.h"

RTTI_IMPL( ProxyGesture, IGesture );
REG_SERIALIZABLE( ProxyGesture );

const float GESTURE_TIMEOUT = 30.0f;

ProxyGesture::ProxyGesture( const std::string & a_GestureId, const std::string & a_InstanceId, bool a_bOverride, const std::string & a_Origin ) :
	IGesture( a_GestureId ),
	m_InstanceId( a_InstanceId ),
	m_Origin( a_Origin ),
	m_bOverride( a_bOverride )
{}

ProxyGesture::ProxyGesture() : 
	m_bOverride( false )
{}

ProxyGesture::~ProxyGesture()
{}

void ProxyGesture::Serialize(Json::Value & json)
{
	IGesture::Serialize( json );
	json["m_Origin"] = m_Origin;
	json["m_InstanceId"] = m_InstanceId;
	json["m_bOverride"] = m_bOverride;

	SerializeVector( "m_Overrides", m_Overrides, json );
}

void ProxyGesture::Deserialize(const Json::Value & json)
{
	IGesture::Deserialize( json );
	m_Origin = json["m_Origin"].asString();
	m_InstanceId = json["m_InstanceId"].asString();
	m_bOverride = json["m_bOverride"].asBool();

	DeserializeVector( "m_Overrides", json, m_Overrides );
}

bool ProxyGesture::Start()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );

	// if overrides is true, then we find any existing gestures with the same
	// ID and pull them from the gesture manager. We will restore those gestures
	// if we are stopped.
	if ( m_bOverride )
	{
		GestureManager * pManager = pInstance->GetGestureManager();
		assert( pManager != NULL );

		if ( pManager->FindGestures( m_GestureId, m_Overrides ) )
		{
			for( size_t i=0;i<m_Overrides.size();++i)
				m_Overrides[i]->AddOverride();
		}
	}

	return true;
}

bool ProxyGesture::Stop()
{
	// if we have an active request, make sure we pop the request 
	// and clear all other pending request if any
	if ( ActiveRequest() != NULL )
	{
		if ( PopRequest( true ) )
			PopAllRequests();
	}

	// restore overridden gestures..
	if ( m_Overrides.size() > 0 )
	{
		for( size_t i=0;i<m_Overrides.size();++i)
			m_Overrides[i]->RemoveOverride();
		m_Overrides.clear();
	}

	m_spTimeoutTimer.reset();
	return true;
}

bool ProxyGesture::Execute(GestureDelegate a_Callback, const ParamsMap & a_Params)
{
	if ( PushRequest( a_Callback, a_Params ) )
		ExecuteGesture();
	return true;
}

bool ProxyGesture::Abort()
{
	Request * pReq = ActiveRequest();
	if ( pReq == NULL )
		return false;

	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );
	ITopics * pTopics = pInstance->GetTopics();
	assert( pTopics != NULL );

	Json::Value request;
	request["event"] = "abort_gesture";
	request["gestureId"] = m_GestureId;
	request["instanceId"] = m_InstanceId;

	if (! pTopics->Send( m_Origin, "gesture-manager", request.toStyledString() ) )
	{
		Log::Error( "ProxyGesture", "Failed to send gesture event" );
		return false;
	}

	PopAllRequests();
	m_spTimeoutTimer.reset();

	return true;
}

void ProxyGesture::OnExecuteDone( bool a_bError )
{
	if ( PopRequest( a_bError ) )
		ExecuteGesture();	// execute the next gesture queued
	m_spTimeoutTimer.reset();
}

void ProxyGesture::ExecuteGesture()
{
	Request * pReq = ActiveRequest();
	assert( pReq != NULL );
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );
	ITopics * pTopics = pInstance->GetTopics();
	assert( pTopics != NULL );

	Json::Value request;
	request["event"] = "execute_gesture";
	request["gestureId"] = m_GestureId;
	request["instanceId"] = m_InstanceId;
	request["params"]  = pReq->m_Params.GetData();

	if (! pTopics->Send( m_Origin, "gesture-manager", request.toStyledString() ) )
	{
		Log::Error( "ProxyGesture", "Failed to send gesture event" );
		OnExecuteDone( true );
	}
	else if ( TimerPool::Instance() != NULL )
	{
		m_spTimeoutTimer = TimerPool::Instance()->StartTimer(
			VOID_DELEGATE( ProxyGesture, OnTimeout, 
				boost::static_pointer_cast<ProxyGesture>(shared_from_this()) ), GESTURE_TIMEOUT, true, false );
	}
}

void ProxyGesture::OnTimeout()
{
	Log::Error( "ProxyGesture", "Timeout on gesture" );
	OnExecuteDone( true );
}


