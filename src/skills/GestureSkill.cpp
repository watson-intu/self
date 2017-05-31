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


#include "GestureSkill.h"
#include "gestures/IGesture.h"
#include "gestures/GestureManager.h"
#include "utils/SelfException.h"

REG_SERIALIZABLE( GestureSkill );
RTTI_IMPL( GestureSkill, ISkill);

GestureSkill::GestureSkill() : m_bReplaceParams( false )
{}

GestureSkill::GestureSkill(const std::string & a_GestureID) :
	m_GestureId( a_GestureID ),
	m_bReplaceParams( false )
{}

GestureSkill::GestureSkill( const GestureSkill & a_Copy ) :
	m_GestureId( a_Copy.m_GestureId ),
	m_bReplaceParams( a_Copy.m_bReplaceParams )
{}

void GestureSkill::Serialize(Json::Value & json)
{
	ISkill::Serialize( json );

	json["m_GestureId"] = m_GestureId;
	json["m_GestureParams"] = ISerializable::SerializeObject( &m_GestureParams );
	json["m_bReplaceParams"] = m_bReplaceParams;
}

void GestureSkill::Deserialize(const Json::Value & json)
{
	ISkill::Deserialize( json );

	if ( json.isMember( "m_GestureId" ) )
		m_GestureId = json["m_GestureId"].asString();
	if ( json.isMember( "m_GestureParams" ) )
		ISerializable::DeserializeObject( json["m_GestureParams"], &m_GestureParams );
	if ( json.isMember( "m_bReplaceParams" ) )
		m_bReplaceParams = json["m_bReplaceParams"].asBool();
}

//! ISkill interface
bool GestureSkill::CanUseSkill()
{
	return true;
}

void GestureSkill::UseSkill( Delegate<ISkill *> a_Callback, const ParamsMap & a_Params)
{
	Log::Debug( "GestureSkill", "UseSkill() invoked: %s", GetSkillName().c_str() );
	if ( PushRequest( a_Callback, a_Params ) )
		StartGesture();
}

bool GestureSkill::AbortSkill()
{
	if ( m_spGesture )
	{
		m_eState = ISkill::FAILED;

		m_spGesture->Abort();
		PopAllRequests();
		return true;
	}

	return true;
}

ISkill * GestureSkill::Clone()
{
	return new GestureSkill( *this );
}

void GestureSkill::StartGesture()
{
	Request * pRequest = ActiveRequest();
	assert( pRequest != NULL );
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert(pInstance != NULL);

	m_eState = ACTIVE;
	m_GestureParams.Merge( pRequest->m_Params, true );

	bool bSuccess = false;
	bool bFound = false;

	std::vector<IGesture::SP> gestures;
	if ( pInstance->GetGestureManager()->FindGestures( m_GestureId, gestures ) )
	{
		bFound = true;

		// for now, just pick a random gesture..
		m_spGesture = gestures[ rand() % gestures.size() ];

		Log::Debug( "GestureSkill", "Using gesture %s", m_spGesture->GetGestureId().c_str() );
		bSuccess = m_spGesture->Execute(
			DELEGATE( GestureSkill, OnGestureCompleted, const IGesture::Result &, this), m_GestureParams );
	}

	if (! bSuccess )
	{
		if ( bFound )
			Log::Warning( "GestureSkill", "Failed to use gesture %s.", m_GestureId.c_str() );

		m_eState = FAILED;
		if ( PopRequest() )
			StartGesture();
	}

}

void GestureSkill::OnGestureCompleted( const IGesture::Result & a_State )
{
	m_eState = a_State.m_bError ? FAILED : COMPLETED;
	m_spGesture.reset();

	if (PopRequest())
		StartGesture();
}

