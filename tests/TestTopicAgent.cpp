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
#include "topics/TopicManager.h"
#include "utils/Time.h"

class TestTopicAgent : public UnitTest
{
public:
	//! Data
	bool m_bRegisterTested;
	bool m_bPublishTested;
	bool m_bSubscribeTested;
	int m_nSubscriberCount;
	bool m_bPayloadA;
	bool m_bPayloadB;
	bool m_bPayloadC;
	bool m_bQueryTested;

	//! Construction
	TestTopicAgent() : UnitTest( "TestTopicAgent" ),
		m_bRegisterTested( false ), 
		m_bPublishTested(false),
		m_bSubscribeTested(false),
		m_nSubscriberCount(0),
		m_bPayloadA(false),
		m_bPayloadB(false),
		m_bPayloadC(false),
		m_bQueryTested(false)
	{}

	virtual void RunTest()
	{
		ThreadPool pool( 2 );

		TopicManager A;
		A.SetSelfId( "A" );
		A.SetPort( 8080 );
		A.RegisterTopic( "blackboard", "object", DELEGATE( TestTopicAgent, OnSubscriber, const ITopics::SubInfo &, this ) );

		Test( A.Start() );

		TopicManager B;
		B.SetSelfId( "B" );
		B.SetPort( 8081 );
		B.SetParentHost( "ws://localhost:8080" );
		Test( B.Start() );
		B.RegisterTopic( "blackboard", "object", DELEGATE( TestTopicAgent, OnSubscriber, const ITopics::SubInfo &, this ) );
		B.Subscribe( "../blackboard", DELEGATE( TestTopicAgent, OnPayloadA, const ITopics::Payload &, this ) );
		Spin( m_nSubscriberCount, 1 );

		TopicManager C;
		C.SetSelfId( "C" );
		C.SetPort( 8082 );
		C.SetParentHost( "ws://localhost:8080" );
		Test( C.Start() );
		C.RegisterTopic( "blackboard", "object", DELEGATE( TestTopicAgent, OnSubscriber, const ITopics::SubInfo &, this ) );

		C.Subscribe( "../B/blackboard", DELEGATE( TestTopicAgent, OnPayloadB, const ITopics::Payload &, this ) );
		Spin( m_nSubscriberCount, 2 );
		B.Subscribe("../C/blackboard", DELEGATE(TestTopicAgent, OnPayloadA, const ITopics::Payload &, this));
		Spin( m_nSubscriberCount, 3 );

		Test( m_nSubscriberCount == 3 );
		Test( m_bRegisterTested );
		Test( A.GetSubscriberCount("blackboard") == 1 );
		Test( B.GetSubscriberCount("blackboard") == 1 );
		Test( C.GetSubscriberCount("blackboard") == 1 );

		B.Publish( "blackboard", "Publishing to blackboard B." );
		Spin( m_bPayloadB );
		Test( m_bPayloadB );

		A.Publish( "blackboard", "Publishing to blackboard A." );
		Spin( m_bPayloadA );
		Test( m_bPayloadA );

		// test getting persisted data
		C.Publish( "blackboard", "Publishing to blackboard C.", true );
		A.Subscribe( "C/blackboard", DELEGATE( TestTopicAgent, OnPayloadC, const ITopics::Payload &, this ) );
		Spin( m_nSubscriberCount, 4 );
		Test( C.GetSubscriberCount("blackboard") == 2 );

		Spin( m_bPayloadC );
		Test( m_bPayloadC );

		// test query
		C.Query("../B/", DELEGATE(TestTopicAgent, OnQuery, const ITopics::QueryInfo &, this));
		Spin(m_bQueryTested);

		// test unsubscribe 
		A.Unsubscribe( "C/blackboard" );
		Spin( m_nSubscriberCount, 5 );
		Test( C.GetSubscriberCount("blackboard") == 1 );

		// stop A, then try to publish which should automatically unsubscribe them..
		Test( A.Stop() );
		Spin(m_nSubscriberCount, 100, 1.0f );										// need to spin for a second to let it get disconnected
		Test(B.Publish("blackboard", "Publishing to blackboard B (2)."));		// try to publish, the failure will remove the subscriber
		Test(C.Publish("blackboard", "Publishing to blackboard C (2)."));		// try to publish, the failure will remove the subscriber
		Spin( m_nSubscriberCount, 7 );
		Test( B.GetSubscriberCount( "blackboard" ) == 0 );

		Test( C.Stop() );
		Test( B.Stop() );
	}

	void OnQuery(const ITopics::QueryInfo & info)
	{
		Log::Debug("TestTopicAgent", "Success = %d, path = %s, selfId = %s, parentId = %s",
			info.m_bSuccess, info.m_Path.c_str(), info.m_SelfId.c_str(), info.m_ParentId.c_str());
		Test(info.m_bSuccess);
		Test(info.m_Topics.size() > 0);
		m_bQueryTested = true;
	}

	void OnSubscriber( const ITopics::SubInfo & info )
	{
		m_nSubscriberCount += 1;
		Log::Debug("TestTopicAgent", "OnSubscriber(), subs = %d, topic = %s, origin = %s", 
			m_nSubscriberCount, info.m_Topic.c_str(), info.m_Origin.c_str());
		m_bRegisterTested = true;
	}

	void OnPayloadA( const ITopics::Payload & payload )
	{
		Log::Debug( "TestTopicAgent", "OnPayloadA(), topic = %s, data = %s", payload.m_Topic.c_str(), payload.m_Data.c_str() );
		m_bPayloadA = true;
	}
	void OnPayloadB( const ITopics::Payload & payload )
	{
		Log::Debug( "TestTopicAgent", "OnPayloadB(), topic = %s, data = %s", payload.m_Topic.c_str(), payload.m_Data.c_str() );
		m_bPayloadB = true;
	}
	void OnPayloadC( const ITopics::Payload & payload )
	{
		Log::Debug( "TestTopicAgent", "OnPayloadC(), topic = %s, data = %s", payload.m_Topic.c_str(), payload.m_Data.c_str() );
		m_bPayloadC = true;
	}
};

TestTopicAgent TEST_TOPIC_MANAGER;