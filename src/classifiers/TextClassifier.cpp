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


#include "TextClassifier.h"
#include "SelfInstance.h"
#include "sensors/SensorManager.h"
#include "sensors/ISensor.h"
#include "sensors/AudioData.h"
#include "services/ILanguageParser.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Health.h"
#include "blackboard/Text.h"
#include "blackboard/QuestionIntent.h"
#include "blackboard/Say.h"
#include "blackboard/Failure.h"
#include "blackboard/Attention.h"
#include "blackboard/HangOnIntent.h"
#include "utils/Delegate.h"
#include "classifiers/filters/DuplicateFilter.h"

#include "jsoncpp/json/json.h"
#include <fstream>

#define ENABLE_DEBUGGING					0
#define ENABLE_CLASSIFIER_DELETE			1

#pragma warning(disable:4996)

//! how often to check our classifiers
const double CHECK_CLASSIFIERS_INTERVAL = 300.0f;

RTTI_IMPL( ITextClassifierProxy, ISerializable );
RTTI_IMPL_EMBEDDED( ITextClassifierProxy, IClassFilter, ISerializable );

void ITextClassifierProxy::Serialize(Json::Value & json) 
{
	json["m_bPriority"] = m_bPriority;
	SerializeVector("m_Filters", m_Filters, json);
}

void ITextClassifierProxy::Deserialize(const Json::Value & json)
{
	if ( json.isMember("m_bPriority") )
		m_bPriority = json["m_bPriority"].asBool();

	DeserializeVector("m_Filters", json, m_Filters);

	if (m_Filters.size() == 0)
		m_Filters.push_back( IClassFilter::SP(new DuplicateFilter()));
}

REG_SERIALIZABLE( TextClassifier );
RTTI_IMPL( TextClassifier, IClassifier );

RTTI_IMPL_EMBEDDED( TextClassifier, IntentClass, ISerializable);

TextClassifier::TextClassifier() :
	m_EntitiesParser("NaturalLanguageUnderstandingV1"),
	m_PosParser("AlchemyV1"),
	m_MinIntentConfidence(0.7), 
	m_MinMissNodeConfidence(0.5),
	m_HangOnTime( 120.0f ),
	m_LastFailureResponse(Time().GetEpochTime()),
	m_MinFailureResponseInterval(7.0),
	m_bHoldOn(false)
{}

TextClassifier::~TextClassifier()
{}

void TextClassifier::Serialize(Json::Value & json)
{
	IClassifier::Serialize( json );

	json["m_EntitiesParser"] = m_EntitiesParser;
	json["m_PosParser"] = m_PosParser;
	json["m_MinIntentConfidence"] = m_MinIntentConfidence;
	json["m_MinMissNodeConfidence"] = m_MinMissNodeConfidence;
	json["m_MinFailureResponseInterval"] = m_MinFailureResponseInterval;
	json["m_HangOnTime"] = m_HangOnTime;

	SerializeVector( "m_FailureResponses", m_FailureResponses, json );
	SerializeVector( "m_LowConfidenceResponses", m_LowConfidenceResponses, json );
	SerializeVector( "m_ClassifierProxies", m_ClassifierProxies, json );
	SerializeVector("m_IntentClasses", m_IntentClasses, json);
}

void TextClassifier::Deserialize(const Json::Value & json)
{
	IClassifier::Deserialize( json );

	if (json["m_EntitiesParser"].isString() )
		m_EntitiesParser = json["m_EntitiesParser"].asString();
	if (json["m_PosParser"].isString() )
		m_PosParser = json["m_PosParser"].asString();
	if (json["m_MinIntentConfidence"].isNumeric() )
		m_MinIntentConfidence = json["m_MinIntentConfidence"].asDouble();
	if (json["m_MinMissNodeConfidence"].isNumeric() )
		m_MinMissNodeConfidence = json["m_MinMissNodeConfidence"].asDouble();
    if (json["m_MinFailureResponseInterval"].isNumeric() )
	    m_MinFailureResponseInterval = json["m_MinFailureResponseInterval"].asDouble();
	if ( json["m_HangOnTime"].isNumeric() )
		m_HangOnTime = json["m_HangOnTime"].asFloat();

	DeserializeVector("m_FailureResponses", json, m_FailureResponses);
	DeserializeVector("m_LowConfidenceResponses", json, m_LowConfidenceResponses);
	DeserializeVector("m_ClassifierProxies", json, m_ClassifierProxies);
	DeserializeVector("m_IntentClasses", json, m_IntentClasses);		

	if (m_IntentClasses.size() == 0)
	{
		m_IntentClasses.push_back(IntentClass("negative_feedback", "LearningIntent"));
		m_IntentClasses.push_back(IntentClass("positive_feedback", "LearningIntent"));
		m_IntentClasses.push_back(IntentClass("weather*", "WeatherIntent"));
		m_IntentClasses.push_back(IntentClass("forget*", "LearningIntent"));
		m_IntentClasses.push_back(IntentClass("learn*", "LearningIntent"));
		m_IntentClasses.push_back(IntentClass("hang_on", "HangOnIntent"));
		m_IntentClasses.push_back(IntentClass("dialog*", "QuestionIntent"));
		m_IntentClasses.push_back(IntentClass("question*", "QuestionIntent"));
		m_IntentClasses.push_back(IntentClass("request*", "RequestIntent"));
		m_IntentClasses.push_back(IntentClass("*", "RequestIntent"));
	}
}

const char * TextClassifier::GetName() const
{
	return "TextClassifier";
}

bool TextClassifier::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );
	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert( pBlackboard != NULL );

	pBlackboard->SubscribeToType(Text::GetStaticRTTI(),
		DELEGATE( TextClassifier, OnText, const ThingEvent &, this), TE_ADDED);
	pBlackboard->SubscribeToType( Health::GetStaticRTTI(),
		DELEGATE( TextClassifier, OnHealth, const ThingEvent &, this ), TE_ADDED );
	pBlackboard->SubscribeToType( HangOnIntent::GetStaticRTTI(),
		DELEGATE( TextClassifier, OnHangOn, const ThingEvent &, this ), TE_ADDED );

	for (size_t i = 0; i < m_ClassifierProxies.size(); ++i)
	{
		if (! m_ClassifierProxies[i] )
			continue;

		m_ClassifierProxies[i]->Start();
	}
	
	if ( m_ClassifierProxies.size() == 0 )
		Log::Warning("TextClassifier", "No TextClassifier Proxies loaded");

	Log::Status("TextClassifier", "TextClassifier started");
	return true;
}

bool TextClassifier::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );
	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert( pBlackboard != NULL );

	for (size_t i = 0; i < m_ClassifierProxies.size(); ++i)
	{
		if (! m_ClassifierProxies[i] )
			continue;

		m_ClassifierProxies[i]->Stop();
	}

	pBlackboard->UnsubscribeFromType(Text::GetStaticRTTI(), this);
	pBlackboard->UnsubscribeFromType(Health::GetStaticRTTI(), this);
	pBlackboard->UnsubscribeFromType(HangOnIntent::GetStaticRTTI(), this );

	Log::Status("TextClassifier", "TextClassifier stopped");
	return true;
}

void TextClassifier::OnHealth( const ThingEvent & a_Event )
{
	Health::SP spHealth = DynamicCast<Health>( a_Event.GetIThing() );
	if ( spHealth && spHealth->GetHealthName() == "RemoteNetwork" )
	{
		if ( !spHealth->IsError() )
		{
			Log::Status( "TextClassifier", "Network is up, re-starting TextClassifier proxies." );
			for (size_t i = 0; i < m_ClassifierProxies.size(); ++i)
			{
				if (! m_ClassifierProxies[i] )
					continue;

				m_ClassifierProxies[i]->Start();
			}
		}
	}
}

void TextClassifier::OnHangOn(const ThingEvent & a_Event)
{
	TimerPool * pTimerPool = TimerPool::Instance();
	if ( pTimerPool != NULL )
	{
		Log::Status( "TextClassifier", "Setting hold on state to true." );
		SetHoldOnState( true );
		m_HangOnTimer = pTimerPool->StartTimer( VOID_DELEGATE( TextClassifier, OnHangOnEnd, this ), 
			m_HangOnTime, true, false );
	}
}

void TextClassifier::OnHangOnEnd()
{
	Log::Status( "TextClassifier", "Clearing hold on state." );
	SetHoldOnState( false );
}

void TextClassifier::OnAttention(const ThingEvent & a_Event )
{
	Attention::SP spAttention = DynamicCast<Attention>( a_Event.GetIThing() );
	if(spAttention)
	{
		m_MinIntentConfidence = spAttention->GetMinIntentConfidence();
		m_MinMissNodeConfidence = spAttention->GetMinMissNodeConfidence();
	}
}
void TextClassifier::OnText(const ThingEvent & a_ThingEvent)
{
	Text::SP spText = DynamicCast<Text>(a_ThingEvent.GetIThing());
	if (spText )
	{
        if ( spText->ClassifyIntent() )
			new ClassifyText(this, spText);
	}
}

const std::string & TextClassifier::GetFailureResponse()
{
	if ( m_FailureResponses.size() > 0 )
		return m_FailureResponses[ rand() % m_FailureResponses.size() ];

	static std::string DEFAULT;
	return DEFAULT;
}

const std::string & TextClassifier::GetLowConfidenceResponse()
{
	if ( m_LowConfidenceResponses.size() > 0 )
		return m_LowConfidenceResponses[ rand() % m_LowConfidenceResponses.size() ];

	static std::string DEFAULT;
	return DEFAULT;
}

//! Helper object for getting speech intent while hanging onto the Speech shared pointer..
TextClassifier::ClassifyText::ClassifyText(TextClassifier * a_pClassifier, Text::SP a_spText) : 
	m_pClassifier( a_pClassifier ),
	m_spText(a_spText),
	m_PendingReq( 0 ),
	m_CompletedReq( 0 ),
	m_bIntentCreated(false)
{
	Log::Debug( "TextClassifier", "Classifying: %s", m_spText->GetText().c_str() );

	m_PendingReq += 1;

	ILanguageParser * pEntityParser = Config::Instance()->FindService<ILanguageParser>( m_pClassifier->m_EntitiesParser );
	if ( pEntityParser != NULL && pEntityParser->IsConfigured() )
	{
		m_PendingReq += 1;
		pEntityParser->GetEntities(a_spText->GetText(),
			DELEGATE(ClassifyText, OnTextEntities, const Json::Value &, this));
	}
	ILanguageParser * pPosParser = Config::Instance()->FindService<ILanguageParser>( m_pClassifier->m_PosParser );
	if ( pPosParser != NULL && pPosParser->IsConfigured() )
	{
		m_PendingReq += 1;
		pPosParser->GetPosTags( a_spText->GetText(),
			DELEGATE(ClassifyText, OnTextParsed, const Json::Value &, this ) );
	}

	for (size_t i = 0; i < m_pClassifier->m_ClassifierProxies.size(); i++)
	{
		if (! m_pClassifier->m_ClassifierProxies[i] )
			continue;

		m_PendingReq += 1;
		m_pClassifier->m_ClassifierProxies[i]->ClassifyText( a_spText, 
			DELEGATE(ClassifyText, OnTextClassified, ITextClassifierProxy::ClassifyResult *, this));
	}
	m_CompletedReq += 1;

	if ( m_PendingReq == m_CompletedReq )
		OnClassifyDone();
}

TextClassifier::ClassifyText::~ClassifyText()
{
	for(size_t i=0;i<m_Results.size();++i)
		delete m_Results[i];
}

void TextClassifier::ClassifyText::OnTextClassified( ITextClassifierProxy::ClassifyResult * a_Result )
{
	m_CompletedReq += 1;

	m_Results.push_back( a_Result );

	if ( m_CompletedReq == m_PendingReq || (a_Result->m_bPriority && a_Result->m_fConfidence >= m_pClassifier->m_MinIntentConfidence && !m_Parse.isNull())
			|| ( a_Result->m_pParentProxy != NULL && a_Result->m_pParentProxy->HasFocus()) )
		OnClassifyDone();
}

void TextClassifier::ClassifyText::OnTextParsed( const Json::Value & a_Parse )
{
	m_CompletedReq += 1;
	JsonHelpers::Merge(m_Parse["POS"], a_Parse);
	if ( m_CompletedReq == m_PendingReq )
		OnClassifyDone();
}

void TextClassifier::ClassifyText::OnTextEntities(const Json::Value & a_Parse)
{
	m_CompletedReq += 1;
	JsonHelpers::Merge(m_Parse["Entities"], a_Parse);
	if (m_CompletedReq == m_PendingReq)
		OnClassifyDone();
}


void TextClassifier::ClassifyText::OnClassifyDone()
{
	if (! m_bIntentCreated )
	{
		Log::Debug( "TextClassifier", "OnClassifyDone: %s (%.1f seconds)", 
			m_spText->GetText().c_str(), Time().GetEpochTime() - m_CreateTime.GetEpochTime() );

		// set flag so we only make our intent once..OnClassifyDone() may get called more than once due to a priority proxy
		m_bIntentCreated = true;	

		// find best result..
		ITextClassifierProxy::ClassifyResult * pTopResult = NULL;
		for (size_t i = 0; i < m_Results.size(); ++i)
		{
			if ( pTopResult == NULL || m_Results[i]->m_fConfidence > pTopResult->m_fConfidence
				 || ( m_Results[i]->m_pParentProxy != NULL && m_Results[i]->m_pParentProxy->HasFocus()) )
			{
				pTopResult = m_Results[i];
				// if Conversation is sticky -- use it!
				if( m_Results[i]->m_pParentProxy != NULL && m_Results[i]->m_pParentProxy->HasFocus() )
					break;
			}
		}

		if ( pTopResult != NULL )
		{
			if (pTopResult->m_fConfidence >= m_pClassifier->m_MinIntentConfidence ||
						( pTopResult->m_pParentProxy != NULL && pTopResult->m_pParentProxy->HasFocus() ) )
			{
				IIntent * pIntent = NULL;
				for (size_t i = 0; i < m_pClassifier->m_IntentClasses.size() && pIntent == NULL; ++i)
				{
					const IntentClass & ic = m_pClassifier->m_IntentClasses[i];
					if (StringUtil::WildMatch(ic.m_Intent, pTopResult->m_TopClass ))
					{
						ISerializable * pUncasted = ISerializable::GetSerializableFactory().CreateObject(ic.m_Class);
						if (pUncasted != NULL)
						{
							pIntent = DynamicCast<IIntent>(pUncasted);
							if (pIntent != NULL)
							{
								pIntent->Create( pTopResult->m_Result, m_Parse);
							}
							else
							{
								Log::Error("NLCProxy", "%s is not a IIntent type.", ic.m_Class.c_str());
								delete pUncasted;
							}
						}
						else
						{
							Log::Error("NLCProxy", "Failed to create intent class %s.", ic.m_Class.c_str());
						}
					}
				}

				if ( pIntent != NULL )
				{
					Log::Status( "Conversation", "---- Intent class %s Created, intent: %s, confidence: %.2g / %.2g, text: %s, TextId: %p", 
						pIntent->GetRTTI().GetName().c_str(), 
						pTopResult->m_TopClass.c_str(), 
						pTopResult->m_fConfidence, 
						m_pClassifier->m_MinIntentConfidence, 
						m_spText->GetText().c_str(),
						m_spText.get() );

					// Sticky Conversation
					// Looks for dialog_node in Conversation context
					// If dialog_node's value is root, then Conversation is NOT sticky
					// If dialog_node's value is anything else, then Conversation is indeed sticky
					// NOTE: an ignored intent (like 'nonsense') will end stickiness
					Json::Value dialogStack( JsonHelpers::Resolve( pTopResult->m_Result, "conversation/context/system/dialog_stack" ) );
					if ( dialogStack.isArray() && dialogStack.size() > 0 )
					{
						// Find out the stack node
						std::string dialogNode = dialogStack[0].asString();

						// if stack node is NOT root -- needs to be sticky
						if( dialogNode != "root" && pTopResult->m_pParentProxy != NULL)
							pTopResult->m_pParentProxy->AddFocus();

						// 	if stack node is root -- remove stickiness
						else if ( pTopResult->m_pParentProxy != NULL )
							pTopResult->m_pParentProxy->RemoveFocus();

					}
					m_spText->AddChild( IIntent::SP( pIntent ) );
				}
				else
				{
					Log::Warning( "NLCProxy", "Failed to create intent object for class %s", pTopResult->m_TopClass.c_str() );
					m_spText->AddChild( Failure::SP( new Failure( "TextClassifier", 
						m_spText->GetText(), pTopResult->m_fConfidence, m_pClassifier->m_MinIntentConfidence ) ) );
				}
			}
			else if (! m_pClassifier->m_bHoldOn && pTopResult->m_fConfidence >= 0.0 )
			{
				Log::Debug("Conversation", "Confidence too low - top_class: %s, confidence: %.2g / %.2g, text: %s, TextId: %p",
							pTopResult->m_TopClass.c_str(),  pTopResult->m_fConfidence, m_pClassifier->m_MinIntentConfidence, 
							m_spText->GetText().c_str(), m_spText.get() );

				std::string response = m_pClassifier->GetLowConfidenceResponse();
				if ( response.size() > 0 )
					m_spText->AddChild( Say::SP( new Say( response ) ) );

				m_spText->AddChild( Failure::SP( new Failure("TextClassifier", m_spText->GetText(), pTopResult->m_fConfidence, m_pClassifier->m_MinIntentConfidence) ) );
			}	
		}
		else
		{
			Log::Error( "Conversation", "ClassifyText Failed, %u returned intents.", m_Results.size() );
			if ( ((Time().GetEpochTime() - m_pClassifier->m_LastFailureResponse) > m_pClassifier->m_MinFailureResponseInterval ))
			{
				std::string failure = m_pClassifier->GetFailureResponse();
				if ( failure.size() > 0 )
					m_spText->AddChild( Say::SP( new Say( failure ) ) );

				m_pClassifier->m_LastFailureResponse = Time().GetEpochTime();
				m_spText->AddChild( Failure::SP( new Failure("TextClassifier", m_spText->GetText(), 0.0f, m_pClassifier->m_MinIntentConfidence) ) );
			}
		}
	}

	// if all requests are complete, then delete this object..
	if ( m_CompletedReq == m_PendingReq )
		delete this;
}

