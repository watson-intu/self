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

#include "PluginAgent.h"
#include "utils/StringUtil.h"
#include "utils/Config.h"

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

namespace fs = boost::filesystem;

#if defined(_WIN32)
const std::string PREFIX("");
const std::string POSTFIX(".dll");
#elif defined(__APPLE__)
const std::string PREFIX("lib");
const std::string POSTFIX(".dylib");
#else
const std::string PREFIX("lib");
const std::string POSTFIX(".so");
#endif

REG_SERIALIZABLE(PluginAgent);
RTTI_IMPL(PluginAgent, IAgent);

PluginAgent::PluginAgent() :
	m_PluginPath("shared/plugins/"),
	m_ScanInterval(30.0f),
	m_bEnableByDefault(false)
{}

void PluginAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);

	json["m_PluginPath"] = m_PluginPath;
	json["m_ScanInterval"] = m_ScanInterval;
	json["m_bEnableByDefault"] = m_bEnableByDefault;
}

void PluginAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);

	if (json["m_PluginPath"].isString())
		m_PluginPath = json["m_PluginPath"].asString();
	if (json["m_ScanInterval"].isNumeric())
		m_ScanInterval = json["m_ScanInterval"].asFloat();
	if (json["m_bEnableByDefault"].isBool())
		m_bEnableByDefault = json["m_bEnableByDefault"].asBool();
}

//! IAgent interface
bool PluginAgent::OnStart()
{
	Config * pConfig = Config::Instance();
	if (pConfig == NULL)
		return false;
	TimerPool * pTimers = TimerPool::Instance();
	if (pTimers == NULL)
		return false;

	fs::path pluginPath(pConfig->GetStaticDataPath() + m_PluginPath);
	if (!fs::is_directory(pluginPath))
	{
		try {
			fs::create_directories(pluginPath);
		}
		catch (const std::exception & ex)
		{
			Log::Error("PluginAgent", "Caught Exception: %s", ex.what());
			return false;
		}
	}

	m_spScanTimer = pTimers->StartTimer(VOID_DELEGATE(PluginAgent, OnScan, this), m_ScanInterval, false, true);
	OnScan();

	return true;
}

bool PluginAgent::OnStop()
{
	m_spScanTimer.reset();
	return true;
}

void PluginAgent::OnScan()
{
	Config * pConfig = Config::Instance();
	assert(pConfig != NULL);

	std::string pluginPath(pConfig->GetStaticDataPath() + m_PluginPath);

	// check for removed plugins...
	Config::LibraryList libs(pConfig->GetLibraryList());
	for (Config::LibraryList::const_iterator iLib = libs.begin(); iLib != libs.end(); ++iLib)
	{
		std::string lib(*iLib);
		if (!StringUtil::StartsWith(lib, pluginPath, true))
			continue;		// lib is not in our plugins directory, skip those libs..

		if (!fs::exists(fs::path(lib)))
		{
			if (pConfig->RemoveLib(lib))
				Log::Status("PluginAgent", "Removed plugin %s", lib.c_str());
			else
				Log::Warning("PluginAgent", "Failed to remove plugin %s", lib.c_str());
		}
	}

	// scan for new libs..
	AddLibs(pluginPath);
}

void PluginAgent::AddLibs(const std::string & a_Directory)
{
	for (fs::directory_iterator p(a_Directory); p != fs::directory_iterator(); ++p)
	{
#ifdef _WIN32
		std::string path = StringUtil::Format("%S", p->path().c_str());
#else
		std::string path = p->path().c_str();
#endif
		if (fs::is_regular_file(p->status()))
		{
			if (StringUtil::EndsWith(path, POSTFIX, true))
				ThreadPool::Instance()->InvokeOnMain<std::string *>(DELEGATE(PluginAgent, AddLib, std::string *, this), new std::string(path));
		}
		else if (fs::is_directory(p->status()))
		{
			// recurse into the directory..
			AddLibs(path);
		}
	}

}

void PluginAgent::AddLib(std::string * a_pLib)
{
	Config * pConfig = Config::Instance();
	assert(pConfig != NULL);

	std::string lib(*a_pLib);

	// normalize the slash direction to unix style..
	StringUtil::Replace(lib, "\\", "/");

	bool bFoundLib = false;

	const Config::LibraryList & libs = pConfig->GetLibraryList();
	for (Config::LibraryList::const_iterator iLib = libs.begin(); iLib != libs.end() && !bFoundLib; ++iLib)
	{
		if (*iLib == lib)
			bFoundLib = true;
	}

	if (!bFoundLib)
	{
		if (pConfig->AddLib(lib, m_bEnableByDefault))
			Log::Status("PluginAgent", "Added new plugin %s", lib.c_str());
		else
			Log::Warning("PluginAgent", "Failed to add new plugin %s", lib.c_str());
	}

	delete a_pLib;
}

