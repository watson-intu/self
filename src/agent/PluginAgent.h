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


#ifndef SELF_PLUGIN_AGENT_H
#define SELF_PLUGIN_AGENT_H

#include "IAgent.h"
#include "SelfLib.h"

//! This agent scans a given directory for shared libraries and automatically adds or removes them from the library list
class SELF_API PluginAgent : public IAgent
{
public:
	RTTI_DECL();

	//! Construction
	PluginAgent();

	//! ISerializable interface
	void Serialize( Json::Value & json );
	void Deserialize( const Json::Value & json );

	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

private:

	//! Data
	std::string	m_PluginPath;
	float		m_ScanInterval;		// how often to scan for plug ins in seconds
	bool		m_bEnableByDefault;	// are new libraries enabled by default

	TimerPool::ITimer::SP
				m_spScanTimer;

	void		OnScan();
	void		AddLibs( const std::string & a_Directory );
	void		AddLib( std::string * a_pLib );
};

#endif //SELF_PRIVACYAGENT_H
