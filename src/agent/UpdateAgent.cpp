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


#include "UpdateAgent.h"

#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"
#include "SelfInstance.h"
#include "services/IPackageStore.h"
#include "utils/TimerPool.h"

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <time.h>

REG_SERIALIZABLE(UpdateAgent);
RTTI_IMPL(UpdateAgent, IAgent);

void UpdateAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);

	json["m_fUpdateCheckDelay"] = m_fUpdateCheckDelay;
	json["m_SelfPackageName"] = m_SelfPackageName;
	json["m_FoundUpdateResponse"] = m_FoundUpdateResponse;
	json["m_InstallationCompleteResponse"] = m_InstallationCompleteResponse;
	json["m_LastVersionConfirmed"] = m_LastVersionConfirmed;
	json["m_bAllowRecommendedDownload"] = m_bAllowRecommendedDownload;
}

void UpdateAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);

	if (json.isMember("m_fUpdateCheckDelay"))
		m_fUpdateCheckDelay = json["m_fUpdateCheckDelay"].asFloat();
	if (json.isMember("m_SelfPackageName"))
		m_SelfPackageName = json["m_SelfPackageName"].asString();
	if (json.isMember("m_FoundUpdateResponse"))
		m_FoundUpdateResponse = json["m_FoundUpdateResponse"].asString();
	if (json.isMember("m_InstallationCompleteResponse"))
		m_InstallationCompleteResponse = json["m_InstallationCompleteResponse"].asString();
	if (json.isMember("m_LastVersionConfirmed"))
		m_LastVersionConfirmed = json["m_LastVersionConfirmed"].asString();
	if (json.isMember("m_bAllowRecommendedDownload"))
		m_bAllowRecommendedDownload = json["m_bAllowRecommendedDownload"].asBool();
}

bool UpdateAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	IPackageStore * pPackageStore = pInstance->FindService<IPackageStore>();
	if (pPackageStore == NULL)
	{
		Log::Error("UpdateAgent", "No PackageStore service, not starting agent.");
		return false;
	}

	pInstance->GetBlackBoard()->SubscribeToType("Confirm",
		DELEGATE(UpdateAgent, OnConfirmDownload, const ThingEvent &, this), TE_STATE);
	StartTimer();
	return true;
}

bool UpdateAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance == NULL)
		return false;

	pInstance->GetBlackBoard()->UnsubscribeFromType("Confirm", this);
	m_spCheckVersionTimer.reset();
	return true;
}

void UpdateAgent::StartTimer()
{
	m_spCheckVersionTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(UpdateAgent, OnVersionRequest, this),
		m_fUpdateCheckDelay, true, true);
}

void UpdateAgent::OnVersionRequest()
{
	IPackageStore * pPackageStore = Config::Instance()->FindService<IPackageStore>();
	if (pPackageStore != NULL)
	{
		boost::shared_ptr<UpdateAgent> spThis(boost::static_pointer_cast<UpdateAgent>(shared_from_this()));
		pPackageStore->GetVersions(m_SelfPackageName,
			DELEGATE(UpdateAgent, OnGetVersions, const Json::Value &, spThis));
	}
}

void UpdateAgent::OnManualUpgrade()
{
	IPackageStore * pPackageStore = Config::Instance()->FindService<IPackageStore>();
	if (pPackageStore != NULL)
	{
		boost::shared_ptr<UpdateAgent> spThis(boost::static_pointer_cast<UpdateAgent>(shared_from_this()));
		pPackageStore->GetVersions(m_SelfPackageName,
			DELEGATE(UpdateAgent, OnGetManualVersions, const Json::Value &, spThis));
	}
}

void UpdateAgent::OnGetManualVersions(const Json::Value & response)
{
	IPackageStore * pPackageStore = Config::Instance()->FindService<IPackageStore>();
	if (pPackageStore != NULL)
	{
		if (!response.isNull())
		{
			SelfInstance * pInstance = SelfInstance::GetInstance();
			assert(pInstance != NULL);

			bool bUseDev = pInstance->GetLocalConfig().m_bUseDevVersion;
			const std::string & currentVersion = pInstance->GetSelfVersion();
			const std::string & recVersion = bUseDev ? response["devVersion"].asString() : response["recVersion"].asString();

			if (IPackageStore::VersionCompare(recVersion, currentVersion) != 0)
			{
				m_UpdatePackageVersion = recVersion;
				m_DownloadStatus = "PROCESSING";

				boost::shared_ptr<UpdateAgent> spThis(boost::static_pointer_cast<UpdateAgent>(shared_from_this()));
				pPackageStore->DownloadPackage(m_SelfPackageName, m_UpdatePackageVersion,
					DELEGATE(UpdateAgent, OnDownloadPackage, const std::string &, spThis));
			}
			else
			{
				m_DownloadStatus = "INACTIVE";
			}
		}
		else
		{
			Log::Error("UpdateAgent", "Failed to get versions: %s", response.toStyledString().c_str());
		}
	}
}

void UpdateAgent::OnGetVersions(const Json::Value & response)
{
	IPackageStore * pPackageStore = Config::Instance()->FindService<IPackageStore>();
	if (pPackageStore != NULL)
	{
		if (!response.isNull())
		{
			SelfInstance * pInstance = SelfInstance::GetInstance();
			assert(pInstance != NULL);

			bool bUseDev = pInstance->GetLocalConfig().m_bUseDevVersion;
			const std::string & currentVersion = pInstance->GetSelfVersion();
			const std::string & recVersion = bUseDev ? response["devVersion"].asString() : response["recVersion"].asString();
			const std::string & reqVersion = response["reqVersion"].asString();

			Log::Debug("UpdateAgent", "Got version response: %s and current version: %s",
				response.toStyledString().c_str(), currentVersion.c_str());

			bool downloading = false;
			m_DownloadStatus = "INACTIVE";

			if (IPackageStore::VersionCompare(reqVersion, currentVersion) > 0 && !m_bDownloadChecked)
			{
				Log::Debug("UpdateAgent", "Update agent found that a newer required version is available!");
				m_bDownloadChecked = true;
				m_UpdatePackageVersion = reqVersion;
				m_DownloadStatus = "PROCESSING";

				boost::shared_ptr<UpdateAgent> spThis(boost::static_pointer_cast<UpdateAgent>(shared_from_this()));
				pPackageStore->DownloadPackage(m_SelfPackageName, m_UpdatePackageVersion,
					DELEGATE(UpdateAgent, OnDownloadPackage, const std::string &, spThis));
				downloading = true;
			}

			if (!downloading && m_bAllowRecommendedDownload
				&& (m_LastVersionConfirmed.size() == 0 || IPackageStore::VersionCompare(m_LastVersionConfirmed, recVersion) != 0))
			{
				if (IPackageStore::VersionCompare(recVersion, currentVersion) > 0 && !m_bDownloadChecked)
				{
					Log::Debug("UpdateAgent", "Update agent found that a newer recommended version is available!");

					m_LastVersionConfirmed = recVersion;
					Confirm::SP spConfirm(new Confirm("Update", response));

					SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spConfirm);
					spConfirm->AddChild(Say::SP(new Say(m_FoundUpdateResponse)));
				}
			}
		}
		else
		{
			Log::Error("UpdateAgent", "Failed to get versions: %s", response.toStyledString().c_str());
		}
	}

	m_spCheckVersionTimer.reset();
	StartTimer();
}

void UpdateAgent::OnConfirmDownload(const ThingEvent & a_Event)
{
	IPackageStore * pPackageStore = Config::Instance()->FindService<IPackageStore>();
	if (pPackageStore != NULL)
	{
		Confirm::SP spConfirm = DynamicCast<Confirm>(a_Event.GetIThing());
		if (spConfirm && spConfirm->GetConfirmType() == "Update")
		{
			if (spConfirm->IsConfirmed())
			{
				m_bDownloadChecked = true;
				m_UpdatePackageVersion = m_LastVersionConfirmed;

				boost::shared_ptr<UpdateAgent> spThis(boost::static_pointer_cast<UpdateAgent>(shared_from_this()));
				pPackageStore->DownloadPackage(m_SelfPackageName, m_UpdatePackageVersion,
					DELEGATE(UpdateAgent, OnDownloadPackage, const std::string &, spThis));
			}
		}
	}
}

void UpdateAgent::OnDownloadPackage(const std::string & downloadResponse)
{
	if (downloadResponse.size() > 0 && m_UpdatePackageVersion.size() > 0)
	{
		Log::Status("UpdateAgent", "Package %s %s downloaded, %u bytes, writing to storage for update...",
			m_SelfPackageName.c_str(), m_UpdatePackageVersion.c_str(), downloadResponse.size());

		try {
			std::ofstream selfFile;
			selfFile.open(m_SelfPackageName.c_str());
			selfFile.write(downloadResponse.c_str(), sizeof(char) * downloadResponse.size());
			selfFile.close();

			std::ofstream updateVersionFile;
			updateVersionFile.open("SelfVersion");
			updateVersionFile.write(m_UpdatePackageVersion.c_str(), sizeof(char) * m_UpdatePackageVersion.size());
			updateVersionFile.close();

			Log::Status("UpdateAgent", "Save successful, stopping main thread for update..");
			SelfInstance::GetInstance()->GetBlackBoard()->AddThing(Say::SP(new Say(m_InstallationCompleteResponse)));
			m_DownloadStatus = "COMPLETED";

			ThreadPool::Instance()->StopMainThread(2);
		}
		catch (const std::exception & ex)
		{
			Log::Error("UpdateAgent", "Caught Exception: %s", ex.what());
		}
	}
	else
	{
		Log::Error("UpdateAgent", "Failed to download package %s, version %s",
			m_SelfPackageName.c_str(), m_UpdatePackageVersion.c_str());
	}
}
