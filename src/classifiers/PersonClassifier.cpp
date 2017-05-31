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


#define _CRT_SECURE_NO_WARNINGS

#define ENABLE_LOCAL_DETECTION				1
#define ENABLE_LOCAL_THROTTLE				1

#include "PersonClassifier.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Person.h"
#include "services/IVisualRecognition.h"
#include "utils/JpegHelpers.h"

#include <opencv2/opencv.hpp>

REG_SERIALIZABLE(PersonClassifier);
RTTI_IMPL( PersonClassifier, IClassifier );

PersonClassifier::PersonClassifier() :
	m_ServiceId("VisualRecognitionV1"),
	m_PersonClass("person"),
	m_nPeopleSubs( 0 ),
	m_FaceClassifierFile( "shared/opencv/lbpcascade_frontalface.xml" ),
	m_DetectFacesInterval( 0.5 ),
	m_Padding( 0.09375f )
{}

void PersonClassifier::Serialize(Json::Value & json)
{
	IClassifier::Serialize( json );

	json["m_ServiceId"] = m_ServiceId;
	json["m_PersonClass"] = m_PersonClass;
	json["m_FaceClassifierFile"] = m_FaceClassifierFile;
	json["m_DetectFacesInterval"] = m_DetectFacesInterval;
	json["m_Padding"] = m_Padding;
}

void PersonClassifier::Deserialize(const Json::Value & json)
{
	IClassifier::Deserialize( json );

	if( json["m_ServiceId"].isString() )
		m_ServiceId = json["m_ServiceId"].asString();
	if( json["m_PersonClass"].isString() )
		m_PersonClass = json["m_PersonClass"].asString();
	if( json["m_FaceClassifierFile"].isString() )
		m_FaceClassifierFile = json["m_FaceClassifierFile"].asString();
	if( json["m_DetectFacesInterval"].isNumeric() )
		m_DetectFacesInterval = json["m_DetectFacesInterval"].asDouble();
	if( json["m_Padding"].isNumeric() )
		m_Padding = json["m_Padding"].asFloat();
}

const char * PersonClassifier::GetName() const
{
    return "PersonClassifier";
}

bool PersonClassifier::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );

	pInstance->GetBlackBoard()->SubscribeToType("Image",
		DELEGATE( PersonClassifier, OnImage, const ThingEvent &, this ), TE_ADDED );
	pInstance->GetTopicManager()->RegisterTopic( "person-classifier", "image/jpeg", 
		DELEGATE( PersonClassifier, OnPeople, const TopicManager::SubInfo &, this) );

	m_ProcessingMap.clear();
    Log::Status("PersonClassifier", "PersonClassifier started");
	return true;
}

bool PersonClassifier::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );

	pInstance->GetBlackBoard()->UnsubscribeFromType("Image", this );
	pInstance->GetTopicManager()->UnregisterTopic( "person-classifier" );

	Log::Status("PersonClassifier", "Person Classifier stopped");
	return true;
}

void PersonClassifier::OnImage(const ThingEvent & a_ThingEvent)
{
	//Only trigger the finding of image type when the previous image has completed facial detection
	Image::SP spImage = DynamicCast<Image>(a_ThingEvent.GetIThing());
	if (spImage)
	{
		if ( m_ProcessingMap.find( spImage->GetOrigin() ) == m_ProcessingMap.end() )
		{
			ClassifyFaces::SP spClassifier( new ClassifyFaces( this ) );
			m_ProcessingMap[ spImage->GetOrigin() ] = spClassifier;
		}

		m_ProcessingMap[ spImage->GetOrigin() ]->ProcessImage( spImage );
	}
}

void PersonClassifier::OnPeople(const TopicManager::SubInfo & a_Sub )
{
	if ( a_Sub.m_Subscribed )
		m_nPeopleSubs += 1;
	else
		m_nPeopleSubs -= 1;
	assert( m_nPeopleSubs >= 0 );
}

//--------------------------------------------------------

PersonClassifier::ClassifyFaces::ClassifyFaces( PersonClassifier * a_pClassifier ) : 
	m_pClassifier( a_pClassifier ), m_pFaceClassifier( NULL ), m_LastClassify( 0.0 )
{
#if ENABLE_LOCAL_DETECTION
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );

	m_pFaceClassifier = new cv::CascadeClassifier();
	if (! m_pFaceClassifier->load( pInstance->GetStaticDataPath() + m_pClassifier->m_FaceClassifierFile ) )
	{
		Log::Error( "PersonClassifier", "Failed to load %s.", m_pClassifier->m_FaceClassifierFile.c_str() );
		delete m_pFaceClassifier;
		m_pFaceClassifier = NULL;
	}
#endif
}

bool PersonClassifier::ClassifyFaces::ProcessImage(const Image::SP & a_spImage )
{
	if ( m_spImage )
		return false;		// already processing an image
#if ENABLE_LOCAL_THROTTLE
	double now = Time().GetEpochTime();
	double elapsed = now - m_LastClassify;
	if ( elapsed < m_pClassifier->m_DetectFacesInterval )
		return false;		// not time to classify again
	m_LastClassify = now;
#endif

	m_spImage = a_spImage;
#if ENABLE_LOCAL_DETECTION
	if ( m_pFaceClassifier != NULL )
	{
		// do classification in a background thread so we don't block the main thread..
		ThreadPool::Instance()->InvokeOnThread( VOID_DELEGATE( ClassifyFaces, OnClassifyFaces, this ) );
	}
	else
#endif
	{
		IVisualRecognition * pVisualRecognition = Config::Instance()->FindService<IVisualRecognition>( 
			m_pClassifier->m_ServiceId );
		if (pVisualRecognition != NULL)
		{
			pVisualRecognition->DetectFaces(m_spImage->GetContent(),
				DELEGATE(ClassifyFaces, OnFacesDetected, const Json::Value &, this));
		}
		else
			m_spImage.reset();
	}

	return true;
}

void PersonClassifier::ClassifyFaces::OnClassifyFaces()
{
	const std::string & encoded = m_spImage->GetContent();
	std::vector<unsigned char> image( (unsigned char *)encoded.data(), (unsigned char *)encoded.data() + encoded.size() );
	cv::Mat frame = cv::imdecode( image, CV_LOAD_IMAGE_COLOR );

	std::vector<cv::Rect> faces;
	m_pFaceClassifier->detectMultiScale( frame, faces );

	Json::Value result;
	Json::Value & image0 = result["images"][0];
	for(size_t i=0;i<faces.size();++i)
	{
		Json::Value & face = image0["faces"][i];
		// TODO
		//face["age"]["max"] = "25";				
		//face["age"]["min"] = "20";
		//face["gender"]["gender"] = "male";
		Json::Value & face_rec = face["face_location"];
		face_rec["left"] = faces[i].x;
		face_rec["top"] = faces[i].y;
		face_rec["width"] = faces[i].width;
		face_rec["height"] = faces[i].height;
	}

	ThreadPool::Instance()->InvokeOnMain<Json::Value>( DELEGATE( ClassifyFaces, OnClassifyDone, Json::Value, this), result );
}

struct PeopleImage
{
	bool m_ImageLoaded;
	int m_Width;
	int m_Height;
	int m_Depth;
	std::string m_RGB;

	PeopleImage() : m_ImageLoaded( false ), m_Width( 0 ), m_Height( 0 ), m_Depth( 0 )
	{}

	bool DecodeFromJpeg( const std::string & a_Image )
	{
		if (! JpegHelpers::DecodeImage( 
			a_Image.data(), 
			a_Image.size(), 
			m_Width, m_Height, m_Depth, m_RGB ) )
		{
			Log::Error( "PersonClassifier", "Failed to decode image for publishing." );
			return false;
		}

		m_ImageLoaded = true;
		return true;
	}
	bool EncodeToJpeg( std::string & a_Jpeg )
	{
		return JpegHelpers::EncodeImage( m_RGB.data(), m_Width, m_Height, m_Depth, a_Jpeg);
	}

	void SetColor(int x, int y, unsigned int a_Color )
	{
		assert( m_ImageLoaded );
		if ( x >= 0 && y >= 0 && x < m_Width && y < m_Height )
		{
			int offset = (x * m_Depth) + (y * m_Width * m_Depth);
			unsigned char * pPixel = ((unsigned char *)m_RGB.data()) + offset;
			pPixel[0] = (unsigned char)(a_Color & 0xff);
			pPixel[1] = (unsigned char)((a_Color & 0xff00) >> 8);
			pPixel[2] = (unsigned char)((a_Color & 0xff0000) >> 16);
			if ( m_Depth == 4 )
				pPixel[3] = (unsigned char)((a_Color & 0xff000000) >> 24);
		}
	}

	void DrawRectangle( int a_top, int a_left, int a_width, int a_height, unsigned int a_Color )
	{
		assert( m_ImageLoaded );

		for(int x=0;x<a_width;++x)
		{
			for(int y=0;y<a_height;++y)
			{
				SetColor( a_left + x, a_top, a_Color );				//!< draw top
				SetColor( a_left, a_top + y, a_Color );				//!< draw left
				SetColor( a_left + a_width, a_top + y, a_Color );	//!< draw right
				SetColor( a_left + x, a_top + a_height, a_Color );	//!< draw bottom
			}
		}
	}
};

void PersonClassifier::ClassifyFaces::OnClassifyDone( Json::Value a_Result )
{
	OnFacesDetected( a_Result );
}

void PersonClassifier::ClassifyFaces::OnFacesDetected(const Json::Value & json)
{
	//Log::Status( "PersonClassifier", "OnPersonClassified: %s", json.toStyledString().c_str() );
	if (!json.isNull() && json.isMember("images"))
	{
		PeopleImage publish;
		if ( m_pClassifier->m_nPeopleSubs > 0 )
			publish.DecodeFromJpeg( m_spImage->GetContent() );

		const Json::Value & imageArray = json["images"];
		for (size_t i = 0; i < imageArray.size(); ++i)
		{
			if (imageArray[i].isMember("faces"))
			{
				const Json::Value & faceArray = imageArray[i]["faces"];
				for (size_t j = 0; j < faceArray.size(); ++j)
				{
					const Json::Value & face = faceArray[j];
					std::string ageMax = face["age"]["max"].asString();
					std::string ageMin = face["age"]["min"].asString();
					std::string ageRange = ageMin + "-" + ageMax;
					std::string gender = face["gender"]["gender"].asString();

					// extract the face from the image data..
					int padding = static_cast<int>( publish.m_Width * m_pClassifier->m_Padding );
					int left = face["face_location"]["left"].asInt() - padding;
					int top = face["face_location"]["top"].asInt() - padding;
					int width = face["face_location"]["width"].asInt() + (padding * 2);
					int height = face["face_location"]["height"].asInt() + (padding * 2);

					if ( m_pClassifier->m_nPeopleSubs > 0 )
						publish.DrawRectangle( top, left, width, height, 0xff00ff );

					std::vector<float> face_location;
					std::string face_image;

					if (JpegHelpers::ExtractImage(m_spImage->GetContent(), left, top, width, height, face_image, &face_location))
					{
						Person::SP spPerson(new Person());
						spPerson->SetGender(gender);
						spPerson->SetAgeRange(ageRange);
						spPerson->SetFaceImage(face_image);
						spPerson->SetFaceLocation(face_location);
						spPerson->SetOrigin(m_spImage->GetOrigin());
						m_spImage->AddChild(spPerson);

						Log::DebugHigh("PersonClassifier", "Adding person Gender: %s, AgeRange: %s", gender.c_str(), ageRange.c_str());
					}
				}
			}
		}

		if ( m_pClassifier->m_nPeopleSubs > 0 )
		{
			std::string encoded;
			if ( publish.EncodeToJpeg( encoded ) )
			{
				SelfInstance * pInstance = SelfInstance::GetInstance();
				if ( pInstance != NULL )
					pInstance->GetTopicManager()->Publish( "person-classifier", encoded, false, true );
			}
		}
	}
	else
	{
		Log::Error("PersonClassifer", "Invalid response from VisualRecognition: %s", json.toStyledString().c_str());
	}

    //set the processing flag to false here to allow for additional detection of face
	m_spImage.reset();
}
