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


#include "AgentTest.h"
#include "utils/Log.h"

int AgentTest::RunTests( const std::vector<std::string> & a_Tests )
{
	std::vector<std::string> failed;

	int executed = 0;
	for( TestList::iterator iTest = GetTestList().begin(); iTest != GetTestList().end(); ++iTest )
	{
		AgentTest * pRunTest = *iTest;

		if ( a_Tests.size() > 0 )
		{
			bool bTestFound = false;
			for(size_t i=0;i<a_Tests.size();++i)
				if ( pRunTest->GetName() == a_Tests[i] )
				{
					bTestFound = true;
					break;
				}

			if (! bTestFound )
				continue;
		}

#ifndef _DEBUG
		try {
#endif
			// TODO: replace printf with logging system..
			executed += 1;

			Log::Status( "AgentTest", "Running Test %s...", pRunTest->GetName().c_str() );
			pRunTest->RunTest();

			Log::Status( "AgentTest", "...Test %s COMPLETED.", pRunTest->GetName().c_str() );
#ifndef _DEBUG
		}
		catch( const std::exception & e )
		{
			Log::Error( "AgentTest", "Caught Exception: %s", e.what() );
			failed.push_back( pRunTest->GetName() );
			Log::Error("AgentTest", "...Test %s FAILED.",  pRunTest->GetName().c_str());
		}
		catch(...)
		{
			failed.push_back( pRunTest->GetName() );
			Log::Error( "AgentTest", "...Test %s FAILED.", pRunTest->GetName().c_str() );
		}
#endif
	}

	Log::Status( "UnitTest", "Executed %d tests, %u tests failed.", executed, failed.size() );
	for(size_t i=0;i<failed.size();++i)
		Log::Error( "UnitTest", "... %s failed.", failed[i].c_str() );

	return failed.size();
}

