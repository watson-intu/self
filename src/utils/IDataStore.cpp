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


#include "IDataStore.h"
#include "DataStoreSQLL.h"

RTTI_IMPL_BASE( IDataStore );

IDataStore * IDataStore::Create(const std::string & a_DB, const Json::Value & a_Definition)
{
	// TODO: Use some config to determine which data store implementation to use.
	DataStoreSQLL * pStore = new DataStoreSQLL();
	if (!pStore->Start(a_DB, a_Definition))
	{
		delete pStore;
		pStore = NULL;
	}

	return pStore;
}

