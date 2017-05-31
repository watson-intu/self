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


//
// Created by John Andersen on 2/4/16.
//

#include "utils/UnitTest.h"
#include "utils/ThreadPool.h"
#include "utils/Time.h"
#include "gestures/RestGesture.h"

class TestRestGesture : public UnitTest
{

public:
	TestRestGesture() : UnitTest( "TestRestGesture" ), m_ResponseTested( false )
	{}

	virtual void RunTest()
	{
		ThreadPool pool( 1 );
		RestGesture gesture;
		gesture.SetURL( "http://watson-robotgateway-dev.mybluemix.net/webApp/v1/en" );
		gesture.Execute( DELEGATE( TestRestGesture, OnComplete, const IGesture::Result &, this), ParamsMap() );

		Spin( m_ResponseTested );
		Test( m_ResponseTested );
	}

	void OnComplete( const IGesture::Result & a_State )
	{
		Test(! a_State.m_bError );

		m_ResponseTested = true;
	}

	bool m_ResponseTested;
};

TestRestGesture TEST_REST_GESTURE;

