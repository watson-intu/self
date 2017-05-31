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


#include "IAuthenticate.h"
#include "IAvatar.h"
#include "IFaceRecognition.h"
#include "IGateway.h"
#include "ILanguageParser.h"
#include "ILanguageTranslation.h"
#include "ILocation.h"
#include "IMail.h"
#include "INews.h"
#include "IObjectRecognition.h"
#include "ISpeechToText.h"
#include "ITelephony.h"
#include "ITextToSpeech.h"
#include "IToneAnalyzer.h"
#include "IVisualRecognition.h"

RTTI_IMPL( IAuthenticate, IService );

RTTI_IMPL( IAvatar, IService );

RTTI_IMPL( IFaceRecognition, IService );

RTTI_IMPL( IGateway, IService );
	RTTI_IMPL( ServiceAttributes, ISerializable );
	RTTI_IMPL( Service, ISerializable );
	RTTI_IMPL( ServiceList, ISerializable );

RTTI_IMPL( ILanguageTranslation, IService );
	RTTI_IMPL( Translations, ISerializable );
	RTTI_IMPL( Translation, ISerializable );
	RTTI_IMPL( Languages, ISerializable );
	RTTI_IMPL( Language, ISerializable );
	RTTI_IMPL( IdentifiedLanguages, ISerializable );
	RTTI_IMPL( IdentifiedLanguage, ISerializable );

RTTI_IMPL( ILocation, IService );
RTTI_IMPL( IMail, IService );
RTTI_IMPL( INews, IService );
RTTI_IMPL( IObjectRecognition, IService );

RTTI_IMPL( ISpeechToText, IService );
	RTTI_IMPL( SpeechModel, ISerializable );
	RTTI_IMPL( SpeechModels, ISerializable );
	RTTI_IMPL( WordConfidence, ISerializable );
	RTTI_IMPL( TimeStamp, ISerializable );
	RTTI_IMPL( SpeechAlt, ISerializable );
	RTTI_IMPL( SpeechResult, ISerializable );
	RTTI_IMPL( RecognizeResults, ISerializable );

RTTI_IMPL( ITelephony, IService );

RTTI_IMPL( ITextToSpeech, IService );
	RTTI_IMPL( Voice, ISerializable );
	RTTI_IMPL( Voices, ISerializable );
	
RTTI_IMPL( IToneAnalyzer, IService );
	RTTI_IMPL( DocumentTones, ISerializable );

RTTI_IMPL( IVisualRecognition, IService );

