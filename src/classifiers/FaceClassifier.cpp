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


#include "FaceClassifier.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Entity.h"
#include "services/IFaceRecognition.h"

const int CLASSIFY_RETRY_ATTEMPTS = 0;

REG_SERIALIZABLE(FaceClassifier);
RTTI_IMPL( FaceClassifier, IClassifier );

FaceClassifier::FaceClassifier() : m_MinFaceConfidence( 0.7f )
{}

void FaceClassifier::Serialize(Json::Value & json)
{
	IClassifier::Serialize( json );

	json["m_MinFaceConfidence"] = m_MinFaceConfidence;
}

void FaceClassifier::Deserialize(const Json::Value & json)
{
	IClassifier::Deserialize( json );

	if ( json["m_MinFaceConfidence"].isDouble() )
		m_MinFaceConfidence = json["m_MinFaceConfidence"].asFloat();
}

const char * FaceClassifier::GetName() const
{
	return "FaceClassifier";
}

bool FaceClassifier::OnStart()
{
	BlackBoard * pBlackboard = SelfInstance::GetInstance()->GetBlackBoard();
	pBlackboard->SubscribeToType( "Person",
		DELEGATE( FaceClassifier, OnPerson, const ThingEvent &, this ), TE_ADDED );

	Log::Status("FaceClassifier", "FaceClassifier started");
	return true;
}

bool FaceClassifier::OnStop()
{
	BlackBoard * pBlackboard = SelfInstance::GetInstance()->GetBlackBoard();
	pBlackboard->UnsubscribeFromType( "Person", this );
	m_ProcessingMap.clear();

	Log::Status("FaceClassifier", "Person Classifier stopped");
	return true;
}

bool FaceClassifier::LearnPerson( const Person::SP & a_spPerson, const std::string & a_Name )
{
	IFaceRecognition * pService = Config::Instance()->FindService<IFaceRecognition>();
	if ( pService == NULL )
		return false;
	if (! a_spPerson )
		return false;

	if ( a_Name.size() == 0 )
	{
		Log::Error( "FaceClassifier", "Name should be set to teach new face." );
		return false;
	}

	IThing::SP spFeatures = a_spPerson->FindChildType( "Features" );
	if ( !spFeatures )
	{
		Log::Warning( "FaceClassifier", "Person has no face features." );
		return false;
	}

	TiXmlDocument xml;
	xml.Parse( (*spFeatures)["F256"].asCString() );

	if ( xml.Error() )
	{
		Log::Warning( "FaceClassifier", "Person has no face features." );
		return false;
	}

	if (! pService->AddFace( xml, 
		a_spPerson->GetFaceImage(), 
		UniqueID( true ).Get(), 
		a_Name,
		a_spPerson->GetGender(),
		"",							// DOB
		DELEGATE( FaceClassifier, OnLearnPerson, const Json::Value &, this ) ) )
	{
		Log::Error( "FaceClassifier", "Failed to add face to IVA." );
		return false;
	}

	return true;
}

void FaceClassifier::OnPerson(const ThingEvent & a_ThingEvent)
{
	Person::SP spPerson = DynamicCast<Person>(a_ThingEvent.GetIThing());
	if ( spPerson && spPerson->GetFaceImage().size() > 0 )
	{
		Image::SP spImage = spPerson->FindParentType<Image>( true );
		if ( spImage )
		{
			if ( m_ProcessingMap.find( spImage->GetOrigin() ) == m_ProcessingMap.end() )
			{
				SP spThis( boost::static_pointer_cast<FaceClassifier>( shared_from_this() ) );
				m_ProcessingMap[ spImage->GetOrigin() ] = SensorGroup::SP( new SensorGroup( spThis ) );
			}

			m_ProcessingMap[ spImage->GetOrigin() ]->ProcessPerson( spImage, spPerson );
		}
		else
		{
			Log::Error( "FaceClassifier", "Failed to find image parent for person." );
		}
	}
}

void FaceClassifier::OnLearnPerson( const Json::Value & a_Response )
{
	Log::Status( "FaceClassifier", "OnLearnPerson: %s", a_Response.toStyledString().c_str() );
}

//--------------------------------------

bool FaceClassifier::SensorGroup::ProcessPerson( const Image::SP & a_spImage, const Person::SP & a_spPerson )
{
	if ( a_spImage != m_spImage && m_nActive > 0 )
		return false;		// still processing persons from a previous image

	m_spImage = a_spImage;
	new ClassifyFace( shared_from_this(), a_spPerson );

	return true;
}

FaceClassifier::ClassifyFace::ClassifyFace( const SensorGroup::SP & a_pGroup, const Person::SP & a_spPerson ) : 
	m_pGroup( a_pGroup ), m_spPerson( a_spPerson ), m_spThis( this )
{
	m_pGroup->m_nActive += 1;

	IFaceRecognition * pService = Config::Instance()->FindService<IFaceRecognition>();
	if ( pService != NULL && pService->IsConfigured() )
	{
		pService->ClassifyFace( m_spPerson->GetFaceImage(),
			DELEGATE( ClassifyFace, OnFaceClassified, const TiXmlDocument &, m_spThis ) );
	}
	else
	{
		m_spThis.reset();
	}
}

FaceClassifier::ClassifyFace::~ClassifyFace()
{
	m_pGroup->m_nActive -= 1;
}

void FaceClassifier::ClassifyFace::OnFaceClassified( const TiXmlDocument & a_F256 )
{
	IFaceRecognition * pService = Config::Instance()->FindService<IFaceRecognition>();

	bool bError = true;
	if (! a_F256.Error() )
	{
		// save the feature data into the person
		std::stringstream ss; 
		ss << a_F256;

		Json::Value features;
		features["F256"] = ss.str();

		// TODO: We should be able to just search our others graph for a person 
		// with these features..
		m_spPerson->AddChild( IThing::SP( new IThing( TT_PERCEPTION, "Features", features ) ) );

		if ( pService != NULL && pService->IsConfigured() )
		{
			// for now search for the face in the remote DB..
			if (pService->SearchForFace(a_F256, m_pGroup->m_pClassifier->m_MinFaceConfidence, 1,
				DELEGATE(ClassifyFace, OnFaceFound, const Json::Value &, this)))
			{
				bError = false;
			}
		}
	}

	if ( bError )
	{
		// this happens lots if the person is not facing directly towards the camera..
		Log::DebugLow( "FaceClassifier", "Failed to get features for face." );
		m_spThis.reset();
	}
}

void FaceClassifier::ClassifyFace::OnFaceFound( const Json::Value & a_Face )
{
	Log::Debug( "ClassifyFace", "OnFaceFound: %s", a_Face.toStyledString().c_str() );
	if (! a_Face.isNull() && a_Face["items"].isArray() && a_Face["items"].size() > 0 )
	{
		const Json::Value & items = a_Face["items"];
		for(size_t i=0;i<items.size() && i < 1;++i)
		{	// TODO capping RecognizedFace to 1 item for now
			Json::Value item = items[i];
			item["m_Origin"] = m_spPerson->GetOrigin();
			m_spPerson->AddChild(IThing::SP(new IThing(TT_PERCEPTION, "RecognizedFace", item)));
		}
	}
	else
	{
		m_spPerson->AddChild( IThing::SP( new IThing( TT_PERCEPTION, "NewFace" ) ) );
	}

	m_spThis.reset();
}
