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


#include "utils/Config.h"
#include "utils/UnitTest.h"
#include "utils/UniqueID.h"
#include "services/Graph/Graph.h"

class TestGraphService : public UnitTest
{
public:
	bool		m_bGetGraphsTested;
	bool		m_bCreateGraphTested;
	std::string	m_GraphId;
	Graph		m_Graph;
	bool		m_bDeleteGraphTested;
	bool		m_bQueryTested;
	int			m_nVertsCreated;
	int			m_nEdgesCreated;

	//! Construction
	TestGraphService() : UnitTest("TestGraphService"),
		m_GraphId( "sk2_test" ),
		m_bGetGraphsTested( false ),
		m_bCreateGraphTested( false ),
		m_bDeleteGraphTested( false ),
		m_bQueryTested( false ),
		m_nVertsCreated( 0 ),
		m_nEdgesCreated( 0 )
	{}

	struct MakePersonVertex
	{
		TestGraphService *		m_pTest;
		Graph::VertexId			m_Id;			

		MakePersonVertex( TestGraphService * a_pTest, const std::string & a_name, const std::string & a_Sex, int a_Age ) :
			m_pTest( a_pTest )
		{
			Graph::PropertyMap props;
			props["name"] = a_name;
			props["sex"] = a_Sex;
			props["age"] = a_Age;

			m_pTest->m_Graph.Createvertex( m_pTest->m_GraphId, "person", props, 
				DELEGATE( MakePersonVertex, OnVertexCreated, const Json::Value &, this ) );
		}

		void OnVertexCreated( const Json::Value & a_Result )
		{
			Test(! a_Result.isNull() );
			Log::Debug( "TestGremlin", "OnVertexCreated: %s", a_Result.toStyledString().c_str() );

			m_Id = a_Result["result"]["data"][0]["id"].asString();
			m_pTest->m_nVertsCreated += 1;
		}
	};

	virtual void RunTest()
	{
		ThreadPool pool(5);
		TimerPool timers;

		Config config;
		Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);

		if ( config.IsConfigured( m_Graph.GetServiceId() ) )
		{
			Test(m_Graph.Start());
			Test(m_Graph.IsReady());

			m_Graph.GetGraphs( DELEGATE( TestGraphService, OnGetGraphs, const Json::Value &, this ) );
			Spin( m_bGetGraphsTested );
			Test( m_bGetGraphsTested );

			m_GraphId = UniqueID().Get();
			m_Graph.CreateGraph( m_GraphId, DELEGATE( TestGraphService, OnCreateGraph, const Json::Value &, this ) );
			Spin( m_bCreateGraphTested );
			Test( m_bCreateGraphTested );

			bool bWaitForLogs = false;
			Spin( bWaitForLogs, 5.0f );

			MakePersonVertex richard( this, "Richard", "male", 46 );
			MakePersonVertex jj( this, "JJ", "male", 29 );
			MakePersonVertex grady( this, "Grady", "male", 21 );
			MakePersonVertex ray( this, "Ray", "male", 50 );
			MakePersonVertex russell( this, "Russell", "male", 105 );

			Spin( m_nVertsCreated, 5 );
			Test( m_nVertsCreated == 5 );

			Graph::PropertyMap prop;
			prop["weight"] = 1.0f;
			m_Graph.CreateEdge( m_GraphId, richard.m_Id, russell.m_Id, "reports_to", prop, 
				DELEGATE( TestGraphService, OnEdgeCreated, const Json::Value &, this ) );
			m_Graph.CreateEdge( m_GraphId, richard.m_Id, ray.m_Id, "reports_to", prop, 
				DELEGATE( TestGraphService, OnEdgeCreated, const Json::Value &, this ) );
			m_Graph.CreateEdge( m_GraphId, richard.m_Id, jj.m_Id, "team", prop, 
				DELEGATE( TestGraphService, OnEdgeCreated, const Json::Value &, this ) );
			m_Graph.CreateEdge( m_GraphId, jj.m_Id, richard.m_Id, "team", prop, 
				DELEGATE( TestGraphService, OnEdgeCreated, const Json::Value &, this ) );
			m_Graph.CreateEdge( m_GraphId, jj.m_Id, russell.m_Id, "reports_to", prop, 
				DELEGATE( TestGraphService, OnEdgeCreated, const Json::Value &, this ) );
			m_Graph.CreateEdge( m_GraphId, russell.m_Id, ray.m_Id, "reports_to", prop, 
				DELEGATE( TestGraphService, OnEdgeCreated, const Json::Value &, this ) );
			m_Graph.CreateEdge( m_GraphId, ray.m_Id, grady.m_Id, "reports_to", prop, 
				DELEGATE( TestGraphService, OnEdgeCreated, const Json::Value &, this ) );

			Spin( m_nEdgesCreated, 7, 300.0f );
			Test( m_nEdgesCreated == 7 );

			Graph::PropertyMap bindings;
			bindings["findName"] = "Richard";
			m_Graph.Query( m_GraphId, "graph.traversal().V().has('name',findName).out('reports_to');", bindings, 
				DELEGATE( TestGraphService, OnQuery, const Json::Value &, this ) );
			Spin( m_bQueryTested );
			Test( m_bQueryTested );

			// wait for logs to write out..
			Spin( bWaitForLogs, 5.0f );

			// lastly, clean up our test graph..
			m_Graph.DeleteGraph( m_GraphId, DELEGATE( TestGraphService, OnDeleteGraph, const Json::Value &, this ) );
			Spin( m_bDeleteGraphTested );
			Test( m_bDeleteGraphTested );

			Test(m_Graph.Stop() );
		}

	}

	void OnQuery( const Json::Value & a_Result )
	{
		Log::Debug( "TestGremlin", "OnQuery: %s", a_Result.toStyledString().c_str() );
		Test(! a_Result.isNull() );
		m_bQueryTested = true;
	}

	void OnGetGraphs( const Json::Value & a_Result )
	{
		Test(! a_Result.isNull() );
		m_bGetGraphsTested = true;
	}

	void OnCreateGraph( const Json::Value & a_Result )
	{
		Test(! a_Result.isNull() );
		m_GraphId = a_Result["graphId"].asString();
		Test( m_GraphId.size() > 0 );

		m_bCreateGraphTested = true;
	}

	void OnEdgeCreated( const Json::Value & a_Result )
	{
		Test(! a_Result.isNull() );
		Log::Debug( "TestGremlin", "OnEdgeCreated: %s", a_Result.toStyledString().c_str() );

		m_nEdgesCreated += 1;
	}

	void OnDeleteGraph( const Json::Value & a_Result )
	{
		Test(! a_Result.isNull() );
		m_bDeleteGraphTested = true;
	}
};

TestGraphService TEST_GREMLIN;