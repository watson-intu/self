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


#ifndef SELF_ISPEECH_TO_TEXT_H
#define SELF_ISPEECH_TO_TEXT_H

#include <stdint.h>

#include "utils/IService.h"
#include "utils/Sound.h"
#include "SelfLib.h"

struct RecognizeResults;
struct SpeechModels;
struct SpeechAudioData;

class SELF_API ISpeechToText : public IService
{
public:
	RTTI_DECL();

	//! Types
	typedef Delegate<RecognizeResults *>			OnRecognize;
	typedef Delegate<SpeechModels *>				OnGetModels;
	typedef Delegate<const std::string & >			ErrorEvent;
	typedef std::vector<std::string>				ModelList;

	//! Construction
	ISpeechToText(const std::string & a_ServiceId, AuthType a_AuthType = AUTH_BASIC ) : IService( a_ServiceId, a_AuthType )
	{}


	//! Accessors
	virtual bool IsListening() const = 0;
	virtual bool IsConnected() const = 0;

	//! Mutators
	virtual void RefreshConnections() = 0;
	virtual void SetOnError( ErrorEvent a_OnError ) = 0;
	virtual void SetRecognizeModels( const ModelList & a_Models ) = 0;
	virtual void SetEnableTimestamps( bool a_bEnable ) = 0;
	virtual void SetEnableWordConfidence( bool a_bEnable ) = 0;
	virtual void SetEnableContinousRecognition( bool a_bEnable ) = 0;
	virtual void SetEnableInteriumResults( bool a_bEnable ) = 0;
	virtual void SetEnableDetectSilence( bool a_bEnable, float a_fThreshold = 0.03f ) = 0;
	virtual bool SetLearningOptOut(bool a_Flag) = 0;

	//! Start listening mode, the user must provide audio data by calling the OnListen() function
	//! on a regular basis. The user is responsible for the object returned by all the callbacks.
	virtual bool StartListening(OnRecognize callback) = 0;
	//! This should be invoked often with new audio data from the microphone or other audio input device.
	virtual void OnListen(const SpeechAudioData & clip) = 0;
	//! This should be invoked to stop listening.
	virtual bool StopListening() = 0;
	//! This can be invoked when the network is lost but before this class has detected
	//! we have been disconnected.
	virtual void Disconnected() = 0;

	//! Request all speech models from the service. The user is responsible for deleting the object
	//! returned by the callback.
	virtual void GetModels(OnGetModels callback) = 0;
	//! Recognize all speech in the given audio clip, invokes the callback.
	virtual void Recognize(const SpeechAudioData & clip, OnRecognize callback, 
		const std::string & a_RecognizeModel = "en-US_BroadbandModel") = 0;
	virtual void Recognize(const Sound & clip, OnRecognize callback, 
		const std::string & a_RecognizeModel = "en-US_BroadbandModel") = 0;
};

//! Data Models
struct SELF_API SpeechModel : public ISerializable
{
	RTTI_DECL();

	std::string m_Name;
	int			m_Rate;
	std::string m_Language;
	std::string m_Description;
	std::string m_URL;

	virtual void Serialize(Json::Value & json)
	{
		json["name"] = m_Name;
		json["rate"] = m_Rate;
		json["language"] = m_Language;
		json["description"] = m_Description;
		json["url"] = m_URL;
	}

	virtual void Deserialize(const Json::Value & json) 
	{
		m_Name = json["name"].asString();
		m_Rate = json["rate"].asInt();
		m_Language = json["language"].asString();
		m_Description = json["description"].asString();
		m_URL = json["url"].asString();
	}
};

struct SELF_API SpeechModels : ISerializable
{
	RTTI_DECL();

	std::vector<SpeechModel>	m_Models;

	virtual void Serialize(Json::Value & json)
	{
		SerializeVector( "models", m_Models, json );
	}

	virtual void Deserialize(const Json::Value & json) 
	{
		DeserializeVector( "models", json, m_Models );
	}
};

struct SELF_API WordConfidence : public ISerializable
{
	RTTI_DECL();

	std::string m_Word;
	double		m_Confidence;

	virtual void Serialize(Json::Value & json)
	{
		json[0] = m_Word;
		json[1] = m_Confidence;
	}

	virtual void Deserialize(const Json::Value & json) 
	{
		m_Word = json[0].asString();
		m_Confidence = json[1].asDouble();
	}
};

struct SELF_API TimeStamp : public ISerializable
{
	RTTI_DECL();

	std::string m_Word;
	double		m_Start;
	double		m_End;

	virtual void Serialize(Json::Value & json)
	{
		json[0] = m_Word;
		json[1] = m_Start;
		json[2] = m_End;
	}

	virtual void Deserialize(const Json::Value & json) 
	{
		m_Word = json[0].asString();
		m_Start = json[1].asDouble();
		m_End = json[2].asDouble();
	}
};

struct SELF_API SpeechAlt : public ISerializable
{
	RTTI_DECL();

	std::string m_Transcript;
	double		m_Confidence;
	std::vector<TimeStamp> 
		m_Timestamps;
	std::vector<WordConfidence> 
		m_WordConfidence;

	virtual void Serialize(Json::Value & json)
	{
		json["transcript"] = m_Transcript;
		json["confidence"] = m_Confidence;

		SerializeVector( "timestamps", m_Timestamps, json );
		SerializeVector( "word_confidence", m_WordConfidence, json );
	}

	virtual void Deserialize(const Json::Value & json) 
	{
		m_Transcript = json["transcript"].asString();
		m_Confidence = json["confidence"].asDouble();

		DeserializeVector( "timestamps", json, m_Timestamps );
		DeserializeVector( "word_confidence", json, m_WordConfidence );
	}
};

struct SELF_API SpeechResult : public ISerializable
{
	RTTI_DECL();

	bool	m_Final;
	std::vector<SpeechAlt> 
		m_Alternatives;

	virtual void Serialize(Json::Value & json)
	{
		json["final"] = m_Final;
		SerializeVector( "alternatives", m_Alternatives, json );
	}

	virtual void Deserialize(const Json::Value & json) 
	{
		m_Final = json["final"].asBool();
		DeserializeVector( "alternatives", json, m_Alternatives );
	}

};

struct SELF_API RecognizeResults : public ISerializable
{
	RTTI_DECL();

	int m_ResultIndex;
	std::vector<SpeechResult> m_Results;
	std::string m_Language;

	RecognizeResults() : m_ResultIndex( 0 )
	{}

	RecognizeResults( const std::vector<SpeechResult> & results) : m_ResultIndex( 0 )
	{
		m_Results = results;
	}

	bool HasResult() const
	{
		return m_Results.size() > 0 && m_Results[0].m_Alternatives.size() > 0;
	}
	bool HasFinalResult() const
	{
		return HasResult() && m_Results[0].m_Final;
	}
	double GetConfidence() const
	{
		return HasResult() ? m_Results[0].m_Alternatives[0].m_Confidence : -1.0f;
	}

	void SetLanguage(std::string & a_Language)
	{
		m_Language = a_Language;
	}

	std::string & GetLanguage() 
	{
		return m_Language;
	}

	virtual void Serialize(Json::Value & json)
	{
		json["result_index"] = m_ResultIndex;
		SerializeVector( "results", m_Results, json );
	}

	virtual void Deserialize(const Json::Value & json) 
	{
		m_ResultIndex = json["result_index"].asInt();
		DeserializeVector( "results", json, m_Results );
	}
};

struct SELF_API SpeechAudioData : public ISerializable
{
	std::string			m_PCM;
	int					m_Rate;
	int					m_Channels;
	int					m_Bits;
	float				m_Level;

	SpeechAudioData() : m_Rate( 0 ), m_Channels( 0 ), m_Bits( 0 ), m_Level( 0.0f )
	{}

	virtual void Serialize(Json::Value & json)
	{
		json["m_PCM"] = m_PCM;
		json["m_Rate"] = m_Rate;
		json["m_Channels"] = m_Channels;
		json["m_Bits"] = m_Bits;
		json["m_Level"] = m_Level;
	}

	virtual void Deserialize(const Json::Value & json) 
	{
		m_PCM = json["m_PCM"].asString();
		m_Rate = json["m_Rate"].asInt();
		m_Channels = json["m_Channels"].asInt();
		m_Bits = json["m_Bits"].asInt();
		m_Level = json["m_Level"].asFloat();
	}

};


#endif
