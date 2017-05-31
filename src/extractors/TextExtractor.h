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


#ifndef SELF_TEXT_EXTRACTOR_H
#define SELF_TEXT_EXTRACTOR_H

#include <list>
#include <deque>

#include "IExtractor.h"
#include "blackboard/ThingEvent.h"
#include "blackboard/Person.h"
#include "sensors/SensorManager.h"
#include "services/ISpeechToText.h"
#include "services/ILanguageTranslation.h"
#include "utils/Factory.h"
#include "utils/fft/FFT.h"
#include "blackboard/Text.h"

#include "SelfLib.h"

class SelfInstance;

class SELF_API TextExtractor : public IExtractor
{
public:
	RTTI_DECL();

	//! Interface class for filtering Audio before sending to STT 
	class SELF_API IAudioFilter : public ISerializable,
		public boost::enable_shared_from_this<IAudioFilter>
	{
	public:
		RTTI_DECL();

		//! Types
		typedef boost::shared_ptr<IAudioFilter>		SP;
		typedef boost::weak_ptr<IAudioFilter>		WP;

		//! Base class functions
		IAudioFilter() : m_bActive( true )
		{}

		bool IsActive() const
		{
			return m_bActive;
		}
		void SetActiveState(bool a_State)
		{
			m_bActive = a_State;
		}

		//! Interface
		virtual void ApplyFilter(SpeechAudioData & a_Data) = 0;

	protected:
		//! Data
		bool		m_bActive;
	};

	template<typename T>
	boost::shared_ptr<T> FindFilter() const
	{
		for (size_t i = 0; i < m_Filters.size(); ++i)
		{
			boost::shared_ptr<T> spFilter = DynamicCast<T>( m_Filters[i] );
			if (spFilter)
				return spFilter;
		}
		return boost::shared_ptr<T>();
	}

	//! Construction
	TextExtractor();
	~TextExtractor();

	//! ISerialziable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

    //! IFeatureExtractor interface
    virtual const char * GetName() const;
    virtual bool OnStart();
    virtual bool OnStop();

private:
    //! Types
    typedef SensorManager::SensorList			SensorList;
    typedef std::vector<IAudioFilter::SP>		Filters;

	//! Helper Request object to translate a Text object into the back-end language
	class TranslateReq
	{
	public:
		TranslateReq( ILanguageTranslation * a_pTranslation, const IThing::SP & a_spFocus, const Text::SP & a_spText, const std::string & a_TargetLang );

	private:
		void OnTranslate(Translations * a_pTranslation);

		//! Data
		ILanguageTranslation *	m_pTranslation;
		IThing::SP				m_spFocus;
		Text::SP				m_spText;
	};

    //! Data
	IThing::SP			m_spPerceptionRoot;
	IThing::SP			m_spFocus;
	float				m_FocusTimeout;		// how long to keep focus on a given person before we are forced to re-focus on a new person
	float				m_MinConfidence;
	float 				m_MaxConfidence;
	float 				m_ConfidenceThreshold;
	float 				m_ConfidenceThresholdLocal;
	float				m_MinSpeechLevel;
	float 				m_StdDevThreshold;
	float 				m_EnergyTimeInterval;
	float 				m_NameRecognizedThresholdDrop;
	float 				m_MinEnergyAvg;
	float				m_MaxEnergyAvg;
	float 				m_NormalizedEnergyLevel; //Between 0 and 1
	double				m_MinFailureResponseInterval;
	int					m_EnergyAverageSampleCount;
	int 				m_MaxFailureResponsesCount;
	int 				m_BurnInCycles;
	int 				m_FailureResponsesCount;
	double 				m_EnergyLevelCrossedTimestamp;
	float 				m_AgeTimeout;
	int 				m_MinimumAge;
	bool 				m_bStoreAudio;
	std::vector<std::string>
						m_FailureResponses;
	std::deque<float>	m_EnergyLevelAverages;

	TimerPool::ITimer::SP 
						m_spAgeTimeout;

	double				m_LastFailureResponse;
    SensorList     		m_TextSensors;
	ISensor *			m_pAudioSensor;
	bool 				m_Listening;

	Filters				m_Filters;

	int					m_nSpectrumSubs;
	FFT					m_FFT;

	int					m_FFTHeight;
	int					m_FFTWidth;
	bool				m_bAverageMax;

    //! Callback handler
	void OnAddAudio(ISensor * a_pSensor);
	void OnRemoveAudio(ISensor * a_pSensor);
	void OnAddText(ISensor * a_pSensor);
	void OnRemoveText(ISensor * a_pSensor);
	void OnAudioData(IData * data);
    void OnTextData(IData * data);
	void OnPerson( const ThingEvent & a_Event );
	void OnHealth( const ThingEvent & a_Event );
	void OnConversation(const ITopics::Payload & );
    void OnRecognizeSpeech(RecognizeResults * a_pResults);

	void OnSpectrumSubscribe(const ITopics::SubInfo & );
	void PublishSpectrum();
};

#endif //SELF_TEXT_EXTRACTOR_H
