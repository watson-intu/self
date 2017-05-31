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


#ifndef SELF_DATA_STORE_H
#define SELF_DATA_STORE_H

#include "jsoncpp/json/json.h"
#include "utils/Delegate.h"
#include "utils/Logic.h"
#include "utils/RTTI.h"
#include "utils/ISerializable.h"
#include "utils/IConditional.h"
#include "SelfLib.h"

//! This abstract interface defines an interface for storing data and finding that data by indexed fields.
//! Facade design pattern is being used here.
class SELF_API IDataStore
{
public:
	RTTI_DECL();

	typedef std::list<std::string>			QueryResults;
	typedef std::vector<IConditional::SP>	Conditions;

	virtual ~IDataStore()
	{}

	//! Create a data store object using the provided definition.
	//! Definitions are structured JSON data that describe the data that will be stored in
	//! this store. We look for a named JSON at the root called "Indexed_" which is an array
	//! of all data that should be indexed by the store for the Find() function. 

	//! Example Definition:
	//! {
	//!     "_Indexed" : [ "ID", "response/text" ],						// index the ID & text data
	//!     "ID": "",					
	//!     "response": {
	//!          "text": "Hello World"
	//!     }
	//! }
	static IDataStore * Create(
		const std::string & a_DB,
		const Json::Value & a_Definition);

	//! Initialize this data store with the provided definition, this is called automatically by the static Create() function above.
	virtual bool Start(const std::string & a_DB,
		const Json::Value & a_DataDefinition) = 0;
	//! Shutdown this data store, any pending data should be flushed.
	virtual bool Stop() = 0;
	//! Destroy this data store
	virtual bool Drop() = 0;

	//! Store data into this data store.
	virtual bool Save(const std::string & a_ID,
		const Json::Value & a_Data,
		Delegate<bool> a_Callback = Delegate<bool>()) = 0;
	//! Get data from this store by it's primary ID, this is asynchronous and will invoke the callback.
	virtual bool Load(const std::string & a_ID,
		Delegate<Json::Value *> a_Callback) = 0;
	//! Delete data from this store by it's primary ID.
	virtual bool Delete(const std::string & a_ID,
		Delegate<bool> a_Callback = Delegate<bool>()) = 0;

	//! Query for data based on conditions, this will invoke your callback with the ID's of all 
	//! records that match your conditions.
	virtual bool Find(const Conditions & a_Conditions,
		Delegate<QueryResults *> a_Callback) = 0;
};

#endif // SELF_DATA_STORE_H
