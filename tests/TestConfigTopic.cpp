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
#include "SelfInstance.h"
#include "topics/TopicManager.h"

class TestConfigTopic : public AgentTest
{
public:
	bool			m_bListClassesTested;
	bool			m_bListLibsTested;
	bool			m_bDisableLibTested;
	bool			m_bEnableLibTested;
	bool			m_bAddCredTested;
	bool			m_bRemoveCredTested;
	bool			m_bAddObjectTested;
	std::string		m_ObjGUID;
	bool			m_bRemoveObjectTested;

	//! Construction
	TestConfigTopic() : AgentTest("TestConfigTopic"),
		m_bListClassesTested( false ),
		m_bListLibsTested( false ),
		m_bDisableLibTested( false ),
		m_bEnableLibTested( false ),
		m_bAddCredTested( false ),
		m_bRemoveCredTested( false ),
		m_bAddObjectTested( false ),
		m_bRemoveObjectTested( false )
	{}

	virtual void RunTest()
	{
		TopicManager B;
		B.SetSelfId( "B" );
		B.SetPort( 8081 );
		B.SetParentHost( "ws://localhost:9443" );
		Test( B.Start() );
		B.Subscribe( "../config", DELEGATE( TestConfigTopic, OnConfig, const ITopics::Payload &, this ) );

		Json::Value list_classes;
		list_classes["event"] = "list_classes";
		list_classes["base_class"] = "ISensor";

		B.PublishAt( "../config", list_classes.toStyledString() );
		Spin( m_bListClassesTested );
		Test( m_bListClassesTested );

		Json::Value list_libs;
		list_libs["event"] = "list_libs";

		B.PublishAt( "../config", list_libs.toStyledString() );
		Spin( m_bListLibsTested );
		Test( m_bListLibsTested );

		Json::Value disable_lib;
		disable_lib["event"] = "disable_lib";
		disable_lib["lib"] = "iva_plugin";

		B.PublishAt( "../config", disable_lib.toStyledString() );
		Spin( m_bDisableLibTested, 300.0f );
		Test( m_bDisableLibTested );

		Json::Value enable_lib;
		enable_lib["event"] = "enable_lib";
		enable_lib["lib"] = "iva_plugin";

		B.PublishAt( "../config", enable_lib.toStyledString() );
		Spin( m_bEnableLibTested );
		Test( m_bEnableLibTested );

		Json::Value add_cred;
		add_cred["event"] = "add_cred";
		add_cred["config"]["m_ServiceId"] = "TestServiceV1";
		add_cred["config"]["m_User"] = "username";
		add_cred["config"]["m_Password"] = "password";
		add_cred["config"]["m_URL"] = "url";

		B.PublishAt( "../config", add_cred.toStyledString() );
		Spin( m_bAddCredTested );
		Test( m_bAddCredTested );

		Json::Value remove_cred;
		remove_cred["event"] = "remove_cred";
		remove_cred["serviceId"] = "TestServiceV1";

		B.PublishAt( "../config", remove_cred.toStyledString() );
		Spin( m_bRemoveCredTested );
		Test( m_bRemoveCredTested );

		Json::Value add_object;
		add_object["event"] = "add_object";
		add_object["object"]["Type_"] = "IVA";
		add_object["override"] = false;

		B.PublishAt( "../config", add_object.toStyledString() );
		Spin( m_bAddObjectTested );
		Test( m_bAddObjectTested );

		Json::Value rm_object;
		rm_object["event"] = "remove_object";
		rm_object["object_guid"] = m_ObjGUID;

		B.PublishAt( "../config", rm_object.toStyledString() );
		Spin( m_bRemoveObjectTested );
		Test( m_bRemoveObjectTested );

		Json::Value get_config;
		get_config["event"] = "get_config";

	}

	void OnConfig( const ITopics::Payload & a_Payload )
	{
		if ( a_Payload.m_RemoteOrigin[0] == 0 )
		{
			Json::Reader reader( Json::Features::strictMode() );

			Json::Value json;
			Test( reader.parse( a_Payload.m_Data, json ) );

			Test( json["event"].isString() );
			const std::string & ev = json["event"].asString();

			if ( ev == "class_list" )
			{
				const Json::Value & classes = json["classes"];
				for(size_t i=0;i<classes.size();++i)
					Log::Debug( "TestConfigTopic", "Found class %s", classes[i].asCString() );

				m_bListClassesTested = true;
			}
			else if ( ev == "libs_list" )
			{
				Log::Debug( "TestConfigTopic", "List Libs: %s", json.toStyledString().c_str() );
				Test( json["success"].asBool() );
				m_bListLibsTested = true;
			}
			else if ( ev == "lib_disabled" )
			{
				Log::Debug( "TestConfigTopic", "Lib Disabled: %s", json.toStyledString().c_str() );
				Test( json["success"].asBool() );
				m_bDisableLibTested = true;
			}
			else if ( ev == "lib_enabled" )
			{
				Log::Debug( "TestConfigTopic", "Lib Enabled: %s", json.toStyledString().c_str() );
				Test( json["success"].asBool() );
				m_bEnableLibTested = true;
			}
			else if ( ev == "cred_added" )
			{
				Log::Debug( "TestConfigTopic", "Cred Added: %s", json.toStyledString().c_str() );
				Test( json["success"].asBool() );
				m_bAddCredTested = true;
			}
			else if ( ev == "cred_removed" )
			{
				Log::Debug( "TestConfigTopic", "Cred Removed: %s", json.toStyledString().c_str() );
				Test( json["success"].asBool() );
				m_bRemoveCredTested = true;
			}
			else if ( ev == "object_added" )
			{
				Log::Debug( "TestConfigTopic", "Object Added: %s", json.toStyledString().c_str() );
				Test( json["success"].asBool() );
				m_ObjGUID = json["object"]["GUID_"].asString();
				m_bAddObjectTested = true;
			}
			else if ( ev == "object_removed" )
			{
				Log::Debug( "TestConfigTopic", "Object Removed: %s", json.toStyledString().c_str() );
				Test( json["success"].asBool() );
				m_bRemoveObjectTested = true;
			}
		}
	}
};

TestConfigTopic TEST_CONFIG_TOPIC;
