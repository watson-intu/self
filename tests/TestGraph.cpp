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
#include "utils/Config.h"
#include "models/IGraph.h"
#include "topics/TopicManager.h"
#include "services/Graph/Graph.h"

#include "boost/filesystem.hpp"

class TestGraph : public UnitTest
{
public:
	int			m_nGraphLoaded;
	bool		m_bTraverseTested;
	bool		m_bGraphConnected;
	bool		m_bParentReady;

	//! Construction
	TestGraph() : UnitTest("TestGraph"),
		m_nGraphLoaded( 0 ),
		m_bTraverseTested( false ),
		m_bGraphConnected( false ),
		m_bParentReady( false )
	{}

	static Json::Value MakePerson( const char * a_pName, int age, const char * a_pSex )
	{
		Json::Value obj;
		obj["name"] = a_pName;
		obj["age"] = age;
		obj["sex"] = a_pSex;

		return obj;
	}

	virtual void RunTest()
	{
		ThreadPool pool(5);
		TimerPool timers;

		boost::filesystem::remove( "./db/TestGraph.db" );

		Config config;
		Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);
		if ( config.IsConfigured( "GraphV1" ) )
			config.GetService<Graph>();
		Test( config.StartServices() );

		std::string graphId( UniqueID().Get() );
		IGraph::SP spGraph( IGraph::Create( "SelfGraph", graphId ) );
		Test( spGraph->Connect( DELEGATE( TestGraph, OnGraphLoaded, IGraph::SP, this) ) );
		Spin(m_nGraphLoaded, 1);

		spGraph->Clear();		// wipe the data from any previous graph..
		IVertex::SP spRichard = spGraph->CreateVertex( "person", MakePerson( "Richard", 46, "male" ) );
		IVertex::SP spJJ = spGraph->CreateVertex( "person", MakePerson( "JJ", 29, "male" ) );
		IVertex::SP spGrady = spGraph->CreateVertex( "person", MakePerson( "Grady", 61, "male" ) );
		IVertex::SP spRay = spGraph->CreateVertex( "person", MakePerson( "Ray", 55, "male" ) );
		IVertex::SP spRuss = spGraph->CreateVertex( "person", MakePerson( "Russ", 49, "male" ) );
		spRichard->CreateEdge( "reports_to", spRuss );
		spRichard->CreateEdge( "reports_to", spRay );
		spRichard->Connect( "team", spJJ );
		spJJ->CreateEdge( "reports_to", spRuss );
		spRuss->CreateEdge( "reports_to", spRay );
		spRay->CreateEdge( "reports_to", spGrady );

		// test saving...
		Json::Value saved_graph;
		Test( spGraph->Export( saved_graph ) );
		Log::Debug( "TestGraph", "Graph Exported: %s", saved_graph.toStyledString().c_str() );

		Test( spGraph->Close() );

		// test loading..
		IGraph::SP spGraph2( IGraph::Create( "SelfGraph", graphId ));
		Test( spGraph2->Connect( DELEGATE(TestGraph, OnGraphLoaded, IGraph::SP, this)) );
		Spin(m_nGraphLoaded, 2);

		// create a traversal...
		// graph.traversal().V().has('groupId','default').out().has('name',eq('Richard')).outE().has('_label',eq('reports_to')).inV()
		ITraverser::SP spMyBosses = spGraph2->CreateTraverser( EqualityCondition( "name", Logic::EQ, "Richard" ) )
			->Out( LabelCondition( "reports_to" ) );
		// test saving...
		Json::Value traverser_data;
		Test( spMyBosses->Export( traverser_data ) );
		Log::Debug( "TestGraph", "Traverser Saved: %s", traverser_data.toStyledString().c_str() );
		// test loading..
		ITraverser::SP spMyBosssesLoaded( spGraph2->LoadTraverser( traverser_data ) );
		Test( spMyBosssesLoaded.get() != NULL );

		// traverse the graph..
		Test( spMyBosssesLoaded->Start( DELEGATE( TestGraph, OnMyBossses, ITraverser::SP, this ) ) );
		Spin( m_bTraverseTested, 300.0f );

		bool bWait = false;
		Spin( bWait, 5.0f );

		// close and destroy this graph..
		spGraph2->Drop();

		Spin( bWait, 5.0f );
	}

	void OnGraphLoaded(IGraph::SP a_spGraph)
	{
		Test(a_spGraph.get() != NULL );
		m_nGraphLoaded += 1;
	}

	void OnMyBossses( ITraverser::SP a_spResult )
	{
		Test( a_spResult->Size() > 0 );
		for(size_t i=0;i<a_spResult->Size();++i)
		{
			IVertex::SP spVertex = (*a_spResult)[ i ];
			Log::Status( "TestGraph", "MyBosses -- %s", spVertex->ToJson().toStyledString().c_str() );
		}

		m_bTraverseTested = true;
	}
};

TestGraph TEST_GRAPH;