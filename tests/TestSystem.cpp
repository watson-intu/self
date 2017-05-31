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
#include "utils/Log.h"
#include "utils/Time.h"
#include "utils/ThreadPool.h"
#include "sensors/System.h"

class TestSystem : public UnitTest
{
public:
	//! Construction
	TestSystem() : UnitTest( "TestSystem" ), m_HealthReceived( false )
	{}

	virtual void RunTest()
	{
		ThreadPool pool(1);
		TimerPool timers;

		System sensor;
		sensor.SetSystemCheckIntervval( 5.0f );
		sensor.Subscribe( DELEGATE( TestSystem, CheckSystem, IData *, this ) );

		Spin( m_HealthReceived );
		Test( m_HealthReceived );
	}

	void CheckSystem( IData * a_pData )
	{
		Log::Debug( "TestSystem", "Checked system!" );
		m_HealthReceived = true;
	}

	bool m_HealthReceived;

};

TestSystem TEST_SYSTEM;
