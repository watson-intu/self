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


#ifndef ICLASSIFIER_H
#define ICLASSIFIER_H

#include "boost/enable_shared_from_this.hpp"
#include "boost/shared_ptr.hpp"

#include "utils/ISerializable.h"
#include "utils/UniqueID.h"
#include "SelfLib.h"				// include last

class ClassifierManager;
class BlackBoard;

//! This class manages all active classifier instances. This classifiers subscribe to sensors 
//! and add concepts to the BlackBoard object contained by the SelfInstance.
class SELF_API IClassifier : public ISerializable,
	public boost::enable_shared_from_this<IClassifier>
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<IClassifier>			SP;
	typedef boost::weak_ptr<IClassifier>			WP;

	enum State {
		AS_STOPPED,
		AS_RUNNING,
		AS_SUSPENDED
	};

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Construction
	IClassifier() : m_bEnabled( true ), m_eState(AS_STOPPED), m_pManager(NULL), m_Overrides(0)
	{
		NewGUID();
	}
	virtual ~IClassifier()
	{}

	//! Interface
	virtual const char * GetName() const = 0;		// returns the textual name of this classifier
	virtual bool OnStart() = 0;						// invoked when this classifier is added to the manager
	virtual bool OnStop() = 0;						// invoked when this classifier is being removed or shutdown

	//! Mutators
	void SetState(State a_eState)
	{
		m_eState = a_eState;
	}

	void SetClassifierManager(ClassifierManager * a_pManager, bool a_bOverride);

	//! Accessors
	bool IsEnabled() const
	{
		return m_bEnabled;
	}

	State GetState() const
	{
		return m_eState;
	}

	bool IsOverridden() const
	{
		return m_Overrides > 0;
	}

	bool IsActive() const
	{
		if (m_eState == AS_RUNNING)
			return true;

		return false;
	}

	void AddOverride();
	void RemoveOverride();

protected:
	//! Data
	bool				m_bEnabled;
	State				m_eState;
	ClassifierManager * m_pManager;
	int					m_Overrides;
	std::vector< SP >	m_Overriden;
};

#endif


