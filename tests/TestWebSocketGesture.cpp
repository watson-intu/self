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
#include "utils/ThreadPool.h"
#include "utils/Time.h"
#include "utils/IWebServer.h"
#include "gestures/WebSocketGesture.h"

class TestWebSocketGesture : public UnitTest
{

public:
	TestWebSocketGesture() : UnitTest("TestWebSocketGesture"), m_ResponseTested(false)
	{}

	virtual void RunTest()
	{
		ThreadPool pool(1);

		IWebServer * pServer = IWebServer::Create("", 8080);
		pServer->AddEndpoint("/test_ws", DELEGATE(TestWebSocketGesture, OnTestWS, IWebServer::RequestSP, this));
		Test(pServer->Start());

		WebSocketGesture gesture;
		gesture.SetURL("ws://127.0.0.1:8080/test_ws");
		gesture.SetMessage("test");
		gesture.Execute(DELEGATE(TestWebSocketGesture, OnComplete, const IGesture::Result &, this), ParamsMap());

		Spin(m_ResponseTested);
		Test(m_ResponseTested);
	}

	void OnComplete(const IGesture::Result & a_State)
	{
		Test(!a_State.m_bError);

		m_ResponseTested = true;
	}

	void OnTestWS(IWebServer::RequestSP a_spRequest)
	{
		Log::Debug("TestWebSocketGesture", "OnTestWS()");
	}

	bool m_ResponseTested;
};

TestWebSocketGesture TEST_WEB_SOCKET_GESTURE;

