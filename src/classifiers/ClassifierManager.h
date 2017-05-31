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


#ifndef CLASSIFIER_MANAGER_H
#define CLASSIFIER_MANAGER_H

#include <list>

#include "utils/Factory.h"
#include "IClassifier.h"
#include "topics/TopicManager.h"
#include "SelfLib.h"		// include last

//! This class manages all active classifier instances. This classifiers subscribe to sensors 
//! and add concepts to the BlackBoard object contained by the SelfInstance.
class SELF_API ClassifierManager
{
public:
	//! Types
	typedef std::list< IClassifier::SP >	ClassifierList;

	//! Construction
	ClassifierManager();
	~ClassifierManager();

	//! Accessors
	const ClassifierList &		GetClassifierList() const;

	//! Start this manager, initializes all available Classifier objects.
	bool Start();
	//! Stop this manager.
	bool Stop();

	//! Add the classifier to this manager, it takes ownership of the object if accepted.
	bool AddClassifier(const IClassifier::SP & a_spClassifier, bool a_bOverride = false);
	//! Remove a classifier from this manager.
	bool RemoveClassifier(const IClassifier::SP & a_spClassifier);
	//! Find all classifiers with the gifen name
	bool FindClassifiers(const std::string & a_Type, std::vector<IClassifier::SP> & a_Overrides);

	void OnClassifierOverride(IClassifier * a_pClassifier);
	void OnClassifierOverrideEnd(IClassifier * a_pClassifer);

	template<typename T>
	T * FindClassifier() const
	{
		for (ClassifierList::const_iterator iClass = m_Classifiers.begin(); iClass != m_Classifiers.end(); ++iClass)
		{
			T * pClassifier = DynamicCast<T>((*iClass).get());
			if (pClassifier != NULL)
				return pClassifier;
		}
		return NULL;
	}

	template<typename T>
	T * GetClassifier()
	{
		T * pClassifier = FindClassifier<T>();
		if (pClassifier == NULL)
		{
			pClassifier = new T();
			if (pClassifier->OnStart())
			{
				m_Classifiers.push_back(IClassifier::SP(pClassifier));
				return pClassifier;
			}

			delete pClassifier;
			pClassifier = NULL;
		}

		return pClassifier;
	}

private:
	//! Data
	bool						m_bActive;
	ClassifierList				m_Classifiers;
	TopicManager *				m_pTopicManager;

	//! Callbacks
	void					OnSubscriber(const ITopics::SubInfo & a_Info);
	void					OnClassifierEvent(const ITopics::Payload & a_Payload);
};

inline const ClassifierManager::ClassifierList & ClassifierManager::GetClassifierList() const
{
	return m_Classifiers;
}


#endif

