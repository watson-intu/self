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
#include "agent/UpdateAgent.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"
#include "services/IPackageStore.h"

class TestUpdateAgent : public AgentTest
{
public:
	bool			m_bUpdateAgentTested;
	std::string		m_UpdatedResponse;
	std::string		m_PackageName;
	std::string		m_SelfVersion;

	//! Construction
	TestUpdateAgent() : AgentTest("TestUpdateAgent"),
		m_UpdatedResponse(""),
		m_PackageName(""),
		m_SelfVersion(""),
		m_bUpdateAgentTested( false )
	{}

	virtual void RunTest()
	{
		SelfInstance * pInstance = SelfInstance::GetInstance();
		Test( pInstance != NULL );
		BlackBoard * pBlackboard = pInstance->GetBlackBoard();
		Test( pBlackboard != NULL );
		UpdateAgent * pUpdateAgent = pInstance->FindAgent<UpdateAgent>();
		Test( pUpdateAgent != NULL );

		pInstance->GetLocalConfig().m_bUseDevVersion = true;

		IPackageStore * pPackageStore = pInstance->FindService<IPackageStore>();
		Test( pPackageStore != NULL );

		m_SelfVersion =  pInstance->GetSelfVersion();
		
		Json::Value json;
		pUpdateAgent->Serialize(json);
		Test( json.isMember("m_InstallationCompleteResponse") );
		m_UpdatedResponse = json["m_InstallationCompleteResponse"].asString();
		Test( json.isMember("m_SelfPackageName") );
		m_PackageName = json["m_SelfPackageName"].asString();

		pUpdateAgent->OnManualUpgrade();

		// if no update is needed
		pPackageStore->GetVersions(m_PackageName, DELEGATE(TestUpdateAgent, OnVersion, const Json::Value &, this));

		// if an update is needed
		pBlackboard->SubscribeToType( "Say", DELEGATE(TestUpdateAgent, OnSay, const ThingEvent &, this) );

		Spin( m_bUpdateAgentTested );
		Test( m_bUpdateAgentTested || pUpdateAgent->GetDownloadStatus() == "COMPLETED"
			  || pUpdateAgent->GetDownloadStatus() == "PROCESSING" );

		pBlackboard->UnsubscribeFromType( "Say", this );
	}

	void OnSay( const ThingEvent & a_Event )
	{
		Say * pSay = DynamicCast<Say>( a_Event.GetIThing().get() );
		Test( pSay != NULL );

		if ( pSay->GetText() == m_UpdatedResponse )
			m_bUpdateAgentTested = true;
	}

	void OnVersion( const Json::Value & json )
	{
		if(	json.isMember("devVersion") && json["devVersion"].asString() == m_SelfVersion )
			m_bUpdateAgentTested = true;
	}
};

TestUpdateAgent TEST_UPDATE_AGENT;
