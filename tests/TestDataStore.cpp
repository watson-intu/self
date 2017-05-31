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


#include "utils/UnitTest.h"
#include "utils/IDataStore.h"

#include <time.h>

class TestDataStore : public UnitTest
{
public:
	bool m_bSaveTested;
	bool m_bLoadTested;
	bool m_bFindTested;
	bool m_bDeleteTested;
	bool m_bLogicCondTested;

	//! Construction
	TestDataStore() : UnitTest("TestDataStore"),
		m_bSaveTested(false),
		m_bLoadTested(false),
		m_bFindTested(false),
		m_bDeleteTested(false),
		m_bLogicCondTested(false)
	{}

	virtual void RunTest()
	{
		Json::Value indexed;
		indexed[0] = "category";

		Json::Value def;
		def["_Indexed"] = indexed;
		def["ID"] = 0;
		def["payload"] = "Banana";
		def["category"] = "fruit";

		ThreadPool threads( 5 );

		IDataStore * pStore = IDataStore::Create("test", def);
		Test(pStore != NULL);

		Json::Value data1;
		data1["ID"] = 1;
		data1["payload"] = "Apple";
		Test(pStore->Save("A", data1, DELEGATE(TestDataStore, OnSave, bool, this)));
		Spin(m_bSaveTested);
		Test(m_bSaveTested);

		Test(pStore->Load("A", DELEGATE(TestDataStore, OnLoad, Json::Value *, this)));
		Spin(m_bLoadTested);
		Test(m_bLoadTested);

		IDataStore::Conditions conditions;
		conditions.push_back(IConditional::SP(new EqualityCondition("category", Logic::EQ, "fruit")));
		Test(pStore->Find(conditions, DELEGATE(TestDataStore, OnFind, IDataStore::QueryResults *, this)));
		Spin(m_bFindTested);
		Test(m_bFindTested);

		Json::Value data2;
		data2["ID"] = 2;
		data2["payload"] = "Spinach";
		data2["category"] = "veg";
		Test(pStore->Save("B", data2));

		IDataStore::Conditions logicalConditions;
		logicalConditions.push_back(IConditional::SP(new LogicalCondition(Logic::OR, EqualityCondition("category", Logic::EQ, "veg") )));
		Test(pStore->Find(logicalConditions, DELEGATE(TestDataStore, OnFindLogical, IDataStore::QueryResults *, this)));
		Spin(m_bLogicCondTested);
		Test(m_bLogicCondTested);

		Test(pStore->Delete("A", DELEGATE(TestDataStore, OnDelete, bool, this)));
		Spin(m_bDeleteTested);
		Test(m_bDeleteTested);

		Test(pStore->Delete("B"));

		delete pStore;
	}

	void OnSave(bool a_bSuccess)
	{
		m_bSaveTested = true;
		Test(a_bSuccess);
	}

	void OnLoad(Json::Value * a_Data)
	{
		m_bLoadTested = true;
		Test((*a_Data)["payload"].asString() == "Apple");
		Test((*a_Data)["category"].asString() == "fruit");
		delete a_Data;
	}

	void OnFind(IDataStore::QueryResults * a_Results)
	{
		m_bFindTested = true;
		Test(a_Results->size() == 1);
		delete a_Results;
	}

	void OnFindLogical(IDataStore::QueryResults * a_Results)
	{
		m_bLogicCondTested = true;
		Test(a_Results->size() == 2);
		delete a_Results;
	}

	void OnDelete(bool a_bSuccess)
	{
		m_bDeleteTested = true;
		Test(a_bSuccess);
	}
};

TestDataStore TEST_DATA_STORE;