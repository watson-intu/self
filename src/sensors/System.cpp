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


#include "System.h"

#if !defined(_WIN32) && !defined(__APPLE__)
#include <sys/sysinfo.h>
#endif

REG_SERIALIZABLE(System);
RTTI_IMPL(System, ISensor);

void System::Serialize(Json::Value & json)
{
	ISensor::Serialize(json);

	json["m_SystemCheckInterval"] = m_SystemCheckInterval;
	json["m_fFreeMemoryThreshold"] = m_fFreeMemoryThreshold;
	json["m_cpuUsageCommand"] = m_cpuUsageCommand;
	json["m_diskUsageCommand"] = m_diskUsageCommand;
	json["m_freeMemoryCommand"] = m_freeMemoryCommand;
	json["m_lastRebootCommand"] = m_lastRebootCommand;
}

void System::Deserialize(const Json::Value & json)
{
	ISensor::Deserialize(json);

	if (json["m_SystemCheckInterval"].isDouble() )
		m_SystemCheckInterval = json["m_SystemCheckInterval"].asFloat();
	if (json["m_fFreeMemoryThreshold"].isDouble() )
		m_fFreeMemoryThreshold = json["m_fFreeMemoryThreshold"].asFloat();
	if (json["m_cpuUsageCommand"].isString() )
		m_cpuUsageCommand = json["m_cpuUsageCommand"].asString();
	if (json["m_diskUsageCommand"].isString() )
		m_diskUsageCommand = json["m_diskUsageCommand"].asString();
	if (json["m_freeMemoryCommand"].isString() )
		m_freeMemoryCommand = json["m_freeMemoryCommand"].asString();
	if (json["m_lastRebootCommand"].isString() )
		m_lastRebootCommand = json["m_lastRebootCommand"].asString();
}

bool System::OnStart()
{
	Log::Debug("System", "OnStart() invoked.");
	m_SystemCheckTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(System, OnCheckSystem, this),
		m_SystemCheckInterval, true, true);
	return true;
}

void System::OnCheckSystem()
{
	if (!m_bProcessing)
	{
		m_bProcessing = true;
		ThreadPool::Instance()->InvokeOnThread(VOID_DELEGATE(System, DoOnCheckSystem, this));
	}
}

void System::DoOnCheckSystem()
{
#if defined(_WIN32)
	// TODO
	BuildHealthData("cpuUsage", 0.0f, false, false );
#elif defined(__APPLE__ )
	// TODO
	BuildHealthData("cpuUsage", 0.0f, false, false );
#else
	try {
		// cpu usage
		float cpuUsage = atof(ExecuteCommand(m_cpuUsageCommand).c_str());
		BuildHealthData("cpuUsage", cpuUsage, false, false);

		// disk usage
		float diskUsage = atof(ExecuteCommand(m_diskUsageCommand).c_str());
		BuildHealthData("diskUsage", diskUsage, false, false);

		// free memory
		float freeMemory = atof(ExecuteCommand(m_freeMemoryCommand).c_str());
		BuildHealthData("freeMemory", freeMemory, (freeMemory < m_fFreeMemoryThreshold) ? true : false, false);

		// uptime and last restart time
		struct sysinfo s_info;
		int error = sysinfo(&s_info);
		long uptime = 0;
		if (error != 0)
		{
			Log::Error("System", "Code error in getting uptime from sysinfo: %d", error);
		}
		else
		{
			uptime = s_info.uptime / 60;
			std::string lastReboot = StringUtil::Format("lastReboot %s", ExecuteCommand(m_lastRebootCommand).c_str());
			BuildHealthData(lastReboot.c_str(), (float)uptime, false, false);
		}

		Log::DebugHigh("System", "cpuUsage: %f | diskUsage: %f | freeMemory: %f | uptime: %d mins",
			cpuUsage, diskUsage, freeMemory, uptime);
		m_bProcessing = false;
	}
	catch (const std::exception & ex)
	{
		Log::Error("System", "Caught Exception: %s", ex.what());
	}
#endif
}

bool System::OnStop()
{
	Log::Debug("System", "OnStop() invoked.");
	m_SystemCheckTimer.reset();
	return true;
}

void System::OnPause()
{}

void System::OnResume()
{}

std::string System::ExecuteCommand(const std::string & command)
{
	std::string result;

#if !defined(_WIN32) && !defined(__APPLE__)
	char buffer[128];

	FILE *pipe = popen(command.c_str(), "r");
	if (!pipe)
		return result;

	try
	{
		while (!feof(pipe))
		{
			if (fgets(buffer, 128, pipe) != NULL)
				result += buffer;
		}
	}
	catch (const std::exception & ex)
	{
		pclose(pipe);
	}
	pclose(pipe);
#endif

	return result;
}

void System::BuildHealthData(const char * state, float value, bool error, bool raiseAlert)
{
	Json::Value paramsMap;
	paramsMap["state"] = StringUtil::Format(state);
	paramsMap["value"] = value;
	paramsMap["error"] = error;
	paramsMap["raiseAlert"] = raiseAlert;

	ThreadPool::Instance()->InvokeOnMain(DELEGATE(System, SendHealthData, HealthData *, this),
		new HealthData("System", paramsMap));
}

void System::SendHealthData(HealthData * a_pData)
{
	SendData(a_pData);
}
