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
#include "utils/Log.h"
#include "utils/TimerPool.h"
#include "utils/JsonHelpers.h"
#include "utils/UnitTest.h"

#include "AgentTest.h"

#include <time.h>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class TestSelfInstance : public UnitTest
{
public:
	//! Construction
	TestSelfInstance() : UnitTest("TestSelfInstance")
	{}

	virtual void RunTest()
	{
		// remove any files from previous test..
		fs::remove_all( fs::path( "./etc/tests/db" ) );
		fs::remove_all( fs::path( "./etc/tests/cache" ) );

		SelfInstance instance( "unit_test", "./etc/tests/", "./etc/tests/" );
		TimerPool::ITimer::SP spTimer = TimerPool::Instance()->StartTimer( 
			VOID_DELEGATE( TestSelfInstance, StartTest, this ), 30.0f, true, false );

		Test( instance.Run() == 0 );
	}

	void StartTest()
	{
		// run all the AgentTest objects
		Test( AgentTest::RunTests( GetArgs() ) == 0 );
		ThreadPool::Instance()->StopMainThread();
	}
};

TestSelfInstance TEST_SELFINSTANCE;
