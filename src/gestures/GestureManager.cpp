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


#include "SelfInstance.h"
#include "MoveJointGesture.h"
#include "GestureManager.h"
#include "IGesture.h"
#include "ProxyGesture.h"

REG_SERIALIZABLE(GestureManager);
RTTI_IMPL( GestureManager, ISerializable );

GestureManager::GestureManager() : m_bActive( false )
{}

GestureManager::~GestureManager()
{}

void GestureManager::Serialize(Json::Value & json)
{
	int index = 0;
	for( GestureMap::iterator iGesture = m_GestureMap.begin();
		iGesture != m_GestureMap.end(); ++iGesture )
	{
		json["m_GestureMap"][index]["key"] = iGesture->first;
		json["m_GestureMap"][index++]["value"] = ISerializable::SerializeObject( iGesture->second.get() );
	}
}

void GestureManager::Deserialize(const Json::Value & json)
{
	const Json::Value & gestures = json["m_GestureMap"];
	for (Json::ValueConstIterator iObject = gestures.begin(); iObject != gestures.end(); ++iObject)
	{
		const Json::Value & item = *iObject;
		const Json::Value & key = item["key"];
		const Json::Value & value = item["value"];

		IGesture::SP spGesture( ISerializable::DeserializeObject<IGesture>(value) );
		if (!spGesture)
			continue;

		spGesture->SetGestureId( key.asString() );		// make sure the correct ID is in the object
		m_GestureMap.insert(std::pair<std::string, IGesture::SP>(key.asString(), spGesture));
	}
}

bool GestureManager::FindGestures( const std::string & a_GestureId, GestureList & a_Gestures ) const
{
	a_Gestures.clear();
	for( GestureMap::const_iterator iFind = m_GestureMap.find( a_GestureId ); 
		iFind != m_GestureMap.end() && iFind->first == a_GestureId; ++iFind )
	{
		if ( iFind->second->IsEnabled() && !iFind->second->IsOverridden() )
			a_Gestures.push_back( iFind->second );
	}

	return a_Gestures.size() > 0;
}

bool GestureManager::Start()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;
	if ( m_bActive )
		return false;

	m_bActive = true;

	pInstance->GetTopics()->RegisterTopic( "gesture-manager", "application/json",
		DELEGATE( GestureManager, OnSubscriber, const ITopics::SubInfo &, this ) );
	pInstance->GetTopics()->Subscribe( "gesture-manager", 
		DELEGATE( GestureManager, OnGestureEvent, const ITopics::Payload &, this ) );

	// load gestures mapping up..
	const std::string & dataPath = pInstance->GetStaticDataPath();
	const SelfInstance::FileList & gestures = pInstance->GetGestureFiles();
	for (GestureFiles::const_iterator iFile = gestures.begin(); iFile != gestures.end(); ++iFile)
	{
		std::string gestureFile( dataPath + *iFile );
		if (ISerializable::DeserializeFromFile(gestureFile, this) == NULL)
			Log::Error("GestureManager", "Failed to load gestures from %s.", gestureFile.c_str() );
	}

	// initialize all gestures loaded... remove any that fail to start.
	for( GestureMap::iterator iGesture = m_GestureMap.begin(); iGesture != m_GestureMap.end(); ++iGesture )
	{
		IGesture * pGesture = iGesture->second.get();
		if (! pGesture->Start() )
			m_GestureMap.erase( iGesture-- );
	}

	Log::Status( "GestureManager", "GestureManager started." );
	return true;
}

bool GestureManager::Stop()
{
	if ( !m_bActive )
		return false;

	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance != NULL )
	{
		pInstance->GetTopics()->Unsubscribe( "gesture-manager", this );
		pInstance->GetTopics()->UnregisterTopic( "gesture-manager" );
	}

	for (GestureMap::iterator iGesture = m_GestureMap.begin(); iGesture != m_GestureMap.end(); ++iGesture)
	{
		IGesture * pGesture = iGesture->second.get();
		if (!pGesture->Stop())
			Log::Warning("GestureManager", "Failed to stop gesture %s.", pGesture->GetRTTI().GetName().c_str() );
	}

	m_bActive = false;
	Log::Status( "GestureManager", "GestureManager stopped." );
	return true;
}

bool GestureManager::AddGesture( const IGestureSP & a_spGesture )
{
	if (! a_spGesture->Start() )
		return false;

	m_GestureMap.insert( GestureMap::value_type( a_spGesture->GetGestureId(), a_spGesture ) );
	return true;
}

bool GestureManager::RemoveGesture( const IGestureSP & a_spGesture )
{
	for( GestureMap::iterator iFind = m_GestureMap.find( a_spGesture->GetGestureId() ); 
		iFind != m_GestureMap.end() && iFind->first == a_spGesture->GetGestureId(); ++iFind )
	{
		if ( iFind->second == a_spGesture )
		{
			if (! a_spGesture->Stop() )
				return false;

			m_GestureMap.erase( iFind );
			return true;
		}
	}

	return false;
}

void GestureManager::OnSubscriber( const ITopics::SubInfo & a_Info )
{
	if (! a_Info.m_Subscribed )
	{
		// remove all proxy gestures that belong to this origin..
		for( ProxyMap::iterator iProxy = m_ProxyMap.begin(); 
			iProxy != m_ProxyMap.end(); )
		{
			ProxyGesture::SP spProxy = iProxy->second;
			if ( spProxy->GetOrigin() == a_Info.m_Origin )
			{
				Log::Status( "GestureManager", "Removing proxy gesture %s for origin %s", 
					spProxy->GetGestureId().c_str(), a_Info.m_Origin.c_str() );
				RemoveGesture( spProxy );
				m_ProxyMap.erase( iProxy++ );
			}
			else
				++iProxy;
		}
	}
}

void GestureManager::OnGestureEvent( const ITopics::Payload & a_Payload )
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
				if ( ev == "add_gesture_proxy" )
				{
					const std::string & gestureId = json["gestureId"].asString();
					const std::string & instanceId = json["instanceId"].asString();
					std::string proxyKey = gestureId + "/" + instanceId;

					bool bOverride = json["override"].asBool();

					Log::Status( "GestureManager", "Adding proxy gesture %s, override: %s", gestureId.c_str(), bOverride ? "True" : "False" );

					ProxyGesture::SP spProxy( new ProxyGesture( gestureId, instanceId, bOverride, a_Payload.m_RemoteOrigin ) );
					if ( AddGesture( spProxy ) )
						m_ProxyMap[ proxyKey ] = spProxy;
					else
						Log::Error( "GestureManager", "Failed to add gesture %s", gestureId.c_str() );
				}
				else if ( ev == "remove_gesture_proxy" )
				{
					bool bSuccess = false;

					const std::string & gestureId = json["gestureId"].asString();
					const std::string & instanceId = json["instanceId"].asString();
					std::string proxyKey = gestureId + "/" + instanceId;

					ProxyMap::iterator iProxy = m_ProxyMap.find( proxyKey );
					if ( iProxy != m_ProxyMap.end() )
					{
						Log::Status( "GestureManager", "Removing proxy gesture %s", gestureId.c_str() );
						bSuccess = RemoveGesture( iProxy->second );
						if ( bSuccess )
							m_ProxyMap.erase( iProxy );
					}

					if (! bSuccess )
						Log::Warning( "GestureManager", "Failed to remove proxy gesture %s", gestureId.c_str() );
				}
				else if ( ev == "execute_done" )
				{
					const std::string & gestureId = json["gestureId"].asString();
					const std::string & instanceId = json["instanceId"].asString();
					std::string proxyKey = gestureId + "/" + instanceId;
					bool bError = json["error"].asBool();

					ProxyMap::iterator iProxy = m_ProxyMap.find( proxyKey );
					if ( iProxy != m_ProxyMap.end() )
						iProxy->second->OnExecuteDone( bError );
				}
				else if ( ev == "error" )
				{
					const std::string & failed_event = json["failed_event"].asString();
					Log::Error( "GestureManager", "Received error on event: %s", failed_event.c_str() );
				}
			}
		}
	}
}
