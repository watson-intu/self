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


#ifndef SELF_UPDATEAGENT_H
#define SELF_UPDATEAGENT_H

#include <list>

#include "IAgent.h"
#include "utils/Factory.h"
#include "utils/ThreadPool.h"
#include "services/IPackageStore.h"
#include "SelfInstance.h"
#include "blackboard/Confirm.h"


#include "SelfLib.h"


//! Forward Declarations
class SelfInstance;
class PackageStore;

class SELF_API UpdateAgent : public IAgent
{
public:
	RTTI_DECL();

	//! Construction
	UpdateAgent() : 
		m_bDownloadChecked(false), 
		m_bAllowRecommendedDownload(false), 
		m_fUpdateCheckDelay(60.0f)
	{}

	void OnManualUpgrade();
	const std::string & GetDownloadStatus() const { return m_DownloadStatus; }

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IAgent interface
	virtual bool OnStart();
	virtual bool OnStop();

private:
	//! Data
	TimerPool::ITimer::SP		m_spCheckVersionTimer;

	//! Event Handlers
	void StartTimer();
	void OnVersionRequest();
	void OnGetVersions(const Json::Value & response);

	void OnDownloadPackage(const std::string & downloadResponse);
	void OnConfirmDownload(const ThingEvent & a_Event);
	void OnGetManualVersions(const Json::Value & response);

	//! Data
	bool		m_bDownloadChecked;
	bool		m_bAllowRecommendedDownload;
	float		m_fUpdateCheckDelay;

	std::string	m_LastVersionConfirmed;
	std::string	m_SelfPackageName;
	std::string m_UpdatePackageVersion;
	std::string	m_FoundUpdateResponse;
	std::string	m_InstallationCompleteResponse;
	std::string m_DownloadStatus;
};

#endif //SELF_UPDATEAGENT_H