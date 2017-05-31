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


#ifndef FEATURE_MANAGER_H
#define FEATURE_MANAGER_H

#include <list>

#include "utils/Factory.h"
#include "IExtractor.h"
#include "topics/TopicManager.h"
#include "SelfLib.h"			// include last

//! This feature manager manages all Feature extractors, those feature extractors push
//! data into the Blackboard object.
class SELF_API FeatureManager
{
public:
	//! Types
	typedef std::list< IExtractor::SP >	FeatureExtractorList;
	typedef Factory< IExtractor >		FeatureExtractorFactory;

	//! Construction
	FeatureManager();
	~FeatureManager();

	//! Accessors
	const FeatureExtractorList & GetFeatureExtractorList() const;

	//! Start this manager, initializes all available extractor objects.
	bool Start();
	//! Stop this manager.
	bool Stop();
	//! Add the extractor to this manager, it takes ownership of the object if accepted.
	bool AddFeatureExtractor(const IExtractor::SP & a_spExtractor, bool a_bOverride = false);
	//! Remove an extractor from this manager.
	bool RemoveFeatureExtractor(const IExtractor::SP & a_spExtractor);
	//! Find all extractors with the given name
	bool FindExtractors(const std::string & a_Name, std::vector<IExtractor::SP> & a_Overrides);

	void OnFeatureExtractorOverride(IExtractor * a_pFeatureExtractor);
	void OnFeatureExtractorOverrideEnd(IExtractor * a_pFeatureExtractor);

	template<typename T>
	T * FindExtractor() const
	{
		for( FeatureExtractorList::const_iterator iExtractor = m_FeatureExtractors.begin(); 
		iExtractor != m_FeatureExtractors.end(); ++iExtractor )
		{
			T * pExtractor = DynamicCast<T>( (*iExtractor).get() );
			if ( pExtractor != NULL )
				return pExtractor;
		}
		return NULL;
	}

	template<typename T>
	T * GetExtractor() 
	{
		T * pExtractor = FindExtractor<T>();
		if ( pExtractor == NULL )
		{
			pExtractor = new T();
			if ( pExtractor->OnStart() )
			{
				m_FeatureExtractors.push_back( IExtractor::SP( pExtractor ) );
				return pExtractor;
			}

			delete pExtractor;
			pExtractor = NULL;
		}

		return pExtractor;
	}

private:
	//!Data
	bool						m_bActive;
	FeatureExtractorList		m_FeatureExtractors;
	TopicManager *				m_pTopicManager;

	//! Callbacks
	void					OnSubscriber(const ITopics::SubInfo & a_Info);
	void					OnFeatureExtractorEvent(const ITopics::Payload & a_Payload);
};

inline const FeatureManager::FeatureExtractorList & FeatureManager::GetFeatureExtractorList() const
{
	return m_FeatureExtractors;
}

#endif