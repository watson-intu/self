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


#include "DiskAudioSensor.h"
#include "utils/ThreadPool.h"
#include "utils/WatsonException.h"
#include "sensors/AudioData.h"
#include "utils/Sound.h"
#include "utils/TimerPool.h"

#include <fstream>
#include <time.h>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/progress.hpp"
#include <iostream>

//REG_OVERRIDE_SERIALIZABLE( Microphone, DiskAudioSensor );
REG_SERIALIZABLE( DiskAudioSensor );
RTTI_IMPL(DiskAudioSensor, Microphone);

DiskAudioSensor::DiskAudioSensor() : 
	m_CaptureAudio( false ), 
	m_Paused( false ),
	m_Error( false ),
	m_CaptureStopped( true ), 
	m_RecordingsPath( "" )
{
	Log::Status("DiskAudioSensor", "making an audio sensor");
}

DiskAudioSensor::~DiskAudioSensor()
{
	// stop our streaming thread..
	m_CaptureAudio = false;
	while(! m_CaptureStopped )
		tthread::this_thread::yield();

}

void DiskAudioSensor::Serialize(Json::Value & json)
{
	Microphone::Serialize(json);

	json["m_RecordingsPath"] = m_RecordingsPath;
}

void DiskAudioSensor::Deserialize(const Json::Value & json)
{
	Microphone::Deserialize( json );

	if ( json.isMember("m_RecordingsPath") )
		m_RecordingsPath = json["m_RecordingsPath"].asString();
}

bool DiskAudioSensor::OnStart()
{
	if (! m_CaptureAudio )
	{
		m_CaptureAudio = true;
		m_CaptureStopped = false;

		ThreadPool::Instance()->InvokeOnThread<void *>( DELEGATE( DiskAudioSensor, CaptureAudio, void *, this), 0 );
		return true;
	}
	return false;
}

bool DiskAudioSensor::OnStop()
{
	m_CaptureAudio = false;
	while(! m_CaptureStopped )
		tthread::this_thread::yield();
	return true;
}

void DiskAudioSensor::OnPause()
{
	m_Paused = true;
}

void DiskAudioSensor::OnResume()
{
	m_Paused = false;
}

void DiskAudioSensor::CaptureAudio( void * )
{
	std::string pauseData;
	pauseData.append(30000, '\0');

	double time = Time().GetEpochTime() + 5.0;
	while (Time().GetEpochTime() < time)
		tthread::this_thread::yield();

	std::vector<std::string> audioTestFiles;


	boost::filesystem::directory_iterator end_iter;
	for ( boost::filesystem::directory_iterator dir_itr( "./etc/tests/audio_files" );
		  dir_itr != end_iter;
		  ++dir_itr )
	{
		try
		{

			if ( boost::filesystem::is_regular_file( dir_itr->status() ) )
			{
				std::cout << "this is what is being added to the vector: " << dir_itr->path().filename() << "\n";
				audioTestFiles.push_back(dir_itr->path().filename().string());

			}
		}
		catch ( const std::exception & ex )
		{
			Log::Status("DiskAudioSensor", "Caught Exception: %s", ex.what() );
		}
	}

	Log::Status("DiskAudioSensor", "GOT ALL FILES DOING WORK NOW");
	for(std::vector<std::string>::const_iterator it = audioTestFiles.begin(); it != audioTestFiles.end(); ++it)
	{
		Log::Status("DiskAudioSensor", "file name: %s", (*it).c_str());

		while(m_Paused && m_CaptureAudio)
			tthread::this_thread::yield();
		if(!m_CaptureAudio)
			break;

		try {
			std::string fullPath = "./etc/tests/audio_files/" + (*it);
			std::ifstream input( fullPath.c_str(), std::ios::binary );
			std::string buffer((std::istreambuf_iterator<char>(input)),
							   (std::istreambuf_iterator<char>()));
			buffer.append(15000, '\0');

			if((buffer.size() & 1) == 1)
				buffer.append(1, '\0');

			ThreadPool::Instance()->InvokeOnMain<AudioData *>(DELEGATE(DiskAudioSensor, SendAudio, AudioData *, this),
															  new AudioData(buffer, 16000, 1, 16 ));

			double waitUntil = Time().GetEpochTime() + (buffer.size() / (16000.0 * 2.0));
			while (Time().GetEpochTime() < waitUntil)
				tthread::this_thread::yield();

			ThreadPool::Instance()->InvokeOnMain<AudioData *>(DELEGATE(DiskAudioSensor, SendAudio, AudioData *, this),
															  new AudioData(pauseData, 16000, 1, 16 ));

			waitUntil = Time().GetEpochTime() + (pauseData.size() / (16000.0 * 2.0));
			while (Time().GetEpochTime() < waitUntil)
				tthread::this_thread::yield();
		}
		catch(const std::exception & ex )
		{
			Log::Error("DiskAudioSensor", "Caught Exception: %s", ex.what() );
		}
	}

	//Give 30 seconds for things to flush
	time = Time().GetEpochTime();

	Log::Status("DiskAudioSensor", "cleaning up audio sensor");

	while (Time().GetEpochTime() < (time + 30.0))
		tthread::this_thread::yield();

	m_CaptureStopped = true;
}

void DiskAudioSensor::SendAudio( AudioData * a_pData )
{
	// now send the data to all subscribers for this microphone..
	SendData(a_pData);
}