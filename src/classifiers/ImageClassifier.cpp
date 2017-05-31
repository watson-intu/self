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

#include "ImageClassifier.h"
#include "SelfInstance.h"

#include "blackboard/BlackBoard.h"
#include "blackboard/Entity.h"
#include "blackboard/Health.h"
#include "services/IVisualRecognition.h"
#include "utils/ZipFile.h"

const std::string IMAGES_EXT(".zip");
const std::string IMAGES_PATH("cache/image_classifier/");
const float CHECK_RETRAINING_INVERVAL = 5.0f;			// how often to check the classifier when training

//#define WRITE_POSITIVE_IMAGES		1

REG_SERIALIZABLE(ImageClassifier);
RTTI_IMPL(ImageClassifier, IClassifier);

void ImageClassifier::Serialize(Json::Value & json)
{
	IClassifier::Serialize(json);

	json["m_MinClassifyConfidence"] = m_MinClassifyConfidence;
	json["m_MaxCacheSize"] = m_MaxCacheSize;
	json["m_ServiceId"] = m_ServiceId;
	json["m_ClassifierName"] = m_ClassifierName;
	json["m_bForceRetrain"] = m_bForceRetrain;
	json["m_ClassifierId"] = m_ClassifierId;
	json["m_ClassifierDate"] = m_ClassifierDate;
	json["m_fRestartTime"] = m_fRestartTime;
	json["m_bUseDefaultClassifier"] = m_bUseDefaultClassifier;

	SerializeList("m_PendingExamples", m_PendingExamples, json);
}

void ImageClassifier::Deserialize(const Json::Value & json)
{
	IClassifier::Deserialize(json);

	if (json.isMember("m_MinClassifyConfidence"))
		m_MinClassifyConfidence = json["m_MinClassifyConfidence"].asDouble();
	if (json.isMember("m_MaxCacheSize"))
		m_MaxCacheSize = json["m_MaxCacheSize"].asUInt();
	if (json.isMember("m_ServiceId"))
		m_ServiceId = json["m_ServiceId"].asString();
	if (json.isMember("m_ClassifierName"))
		m_ClassifierName = json["m_ClassifierName"].asString();
	if (json.isMember("m_bForceRetrain"))
		m_bForceRetrain = json["m_bForceRetrain"].asBool();
	if (json.isMember("m_ClassifierId"))
		m_ClassifierId = json["m_ClassifierId"].asString();
	if (json.isMember("m_ClassifierDate"))
		m_ClassifierDate = json["m_ClassifierDate"].asString();
	if (json.isMember("m_fRestartTime"))
		m_fRestartTime = json["m_fRestartTime"].asFloat();
	if (json.isMember("m_bUseDefaultClassifier"))
		m_bUseDefaultClassifier = json["m_bUseDefaultClassifier"].asBool();
	if (m_ClassifierName == "default")
		m_ClassifierName = "self";

	DeserializeList("m_PendingExamples", json, m_PendingExamples);
}

const char * ImageClassifier::GetName() const
{
	return "ImageClassifier";
}

bool ImageClassifier::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert(pInstance != NULL);
	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert(pBlackboard != NULL);

	// validate the images path..
	const std::string & instanceData = Config::Instance()->GetInstanceDataPath();
	if (!m_ImagesCache.Initialize(instanceData + IMAGES_PATH, m_MaxCacheSize, 0, IMAGES_EXT))
	{
		Log::Error("ImageClassifier", "Failed to initialize image cache.");
		return false;
	}

	// check for a valid classifier...
	IVisualRecognition * pVR = Config::Instance()->FindService<IVisualRecognition>(m_ServiceId);
	if ( pVR != NULL && pVR->IsConfigured())
	{
		SP spThis(boost::static_pointer_cast<ImageClassifier>(shared_from_this()));
		pVR->GetClassifiers(DELEGATE(ImageClassifier, OnGetClassifiers, const Json::Value &, spThis));
	}
	else
		Log::Warning("ImageClassifier", "IVisualRecognition is not configured.");

	pBlackboard->SubscribeToType("Image",
		DELEGATE(ImageClassifier, OnImage, const ThingEvent &, this), TE_ADDED);
	pBlackboard->SubscribeToType("Health",
		DELEGATE(ImageClassifier, OnHealth, const ThingEvent &, this), TE_ADDED);

	m_ProcessingMap.clear();
	Log::Status("ImageClassifier", "ImageClassifier started");
	return true;
}

bool ImageClassifier::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert(pInstance != NULL);
	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert(pBlackboard != NULL);

	m_ImagesCache.Uninitialize();
	pBlackboard->UnsubscribeFromType("Image", this);
	pBlackboard->UnsubscribeFromType("Health", this);

	m_ProcessingMap.clear();

	Log::Status("ImageClassifier", "Image Classifier stopped");
	return true;
}

void ImageClassifier::OnImage(const ThingEvent & a_ThingEvent)
{
	Image::SP spImage = DynamicCast<Image>(a_ThingEvent.GetIThing());
	if (spImage)
	{
		if (m_ProcessingMap.find(spImage->GetOrigin()) == m_ProcessingMap.end())
		{
			SP spThis(boost::static_pointer_cast<ImageClassifier>(shared_from_this()));
			m_ProcessingMap[spImage->GetOrigin()] = ClassifyImage::SP(new ClassifyImage(spThis));
		}

		m_ProcessingMap[spImage->GetOrigin()]->ProcessImage(spImage);
	}
}

bool ImageClassifier::ClassifyImage::ProcessImage(const Image::SP & a_spImage)
{
	if (m_spImage)
		return false;	// still processing an image

	IVisualRecognition * pVR = Config::Instance()->FindService<IVisualRecognition>(m_pClassifier->m_ServiceId);
	if (pVR != NULL && pVR->IsConfigured())
	{
		std::vector<std::string> classifiers;
		if (m_pClassifier->m_ClassifierId.size() > 0)
			classifiers.push_back(m_pClassifier->m_ClassifierId);
		if (m_pClassifier->m_bUseDefaultClassifier)
			classifiers.push_back("default");

		if (classifiers.size() > 0)
		{
			m_spImage = a_spImage;
			pVR->ClassifyImage(m_spImage->GetContent(), classifiers,
				DELEGATE(ClassifyImage, OnImageClassified, const Json::Value &, shared_from_this()));
		}

		return true;
	}

	return false;
}

void ImageClassifier::ClassifyImage::OnImageClassified(const Json::Value & json)
{
	//Log::Status("ImageClassifier", "OnImageClassified: %s", json.toStyledString().c_str() );
	if (!json.isNull() && json.isMember("images"))
	{
		const Json::Value & images = json["images"];
		for (size_t i = 0; i < images.size(); ++i)
		{
			const Json::Value & image = images[i];
			const Json::Value & classifiers = image["classifiers"];
			for (size_t j = 0; j < classifiers.size(); ++j)
			{
				const Json::Value & classifier = classifiers[j];
				// Log::Status("ImageClassifier", "OnImageClassified: %s", classifier.toStyledString().c_str() );
				if (!classifier.isMember("classes"))
					continue;

				const Json::Value & classes = classifier["classes"];
				for (size_t k = 0; k < classes.size(); ++k)
				{
					const Json::Value & imageClass = classes[k];
					const std::string & text = imageClass["class"].asString();
					double score = atof(imageClass["score"].asString().c_str());
					const std::string & type_hierarchy = imageClass["type_hierarchy"].asString();

					if (score < m_pClassifier->m_MinClassifyConfidence)
						continue;

					Log::Debug("ImageClassifier", "Classified Image: %s, Score: %f",
						text.c_str(), score);

					Entity::SP spEntity(new Entity());
					spEntity->SetTopClass(text);
					spEntity->SetTopScore(score);
					spEntity->SetTypeHierarchy(type_hierarchy);

					m_spImage->AddChild(spEntity);
				}
			}
		}
	}
	else
	{
		Log::Error("ImageClassifier", "Invalid response from IVisualRecognition: %s", 
			json.toStyledString().c_str());
	}

	m_spImage.reset();
}

void ImageClassifier::OnGetClassifiers(const Json::Value & a_Response)
{
	IVisualRecognition * pVR = Config::Instance()->FindService<IVisualRecognition>(m_ServiceId);
	//Log::Status( "ImageClassifier", "OnGetClassifiers: %s", a_Response.toStyledString().c_str() );
	if (!a_Response.isNull())
	{
		bool bClassifierFound = false;

		const Json::Value & classifiers = a_Response["classifiers"];
		for (size_t i = 0; i < classifiers.size(); ++i)
		{
			const Json::Value & classifier = classifiers[i];

			const std::string & classifier_id = classifier["classifier_id"].asString();
			const std::string & name = classifier["name"].asString();
			const std::string & status = classifier["status"].asString();

			if (name != m_ClassifierName)
				continue;

			Log::Status("ImageClassifier", "Found classifier %s, name: %s, status: %s",
				classifier_id.c_str(), name.c_str(), status.c_str());
			if ((status == "ready" || status == "training") && !m_bForceRetrain)
			{
				// grab information on this specific classifier then..
				if (pVR != NULL && pVR->IsConfigured())
				{
					m_nPendingOps += 1;
					pVR->GetClassifier(classifier_id,
						DELEGATE(ImageClassifier, OnGetClassifier, const Json::Value &, this));
				}

				if (classifier_id == m_ClassifierId)
					bClassifierFound = true;
			}
			else if (status == "failed" || m_bForceRetrain)
			{
				Log::Status("ImageClassifier", "Removing classifier %s", classifier_id.c_str());
				if (pVR != NULL && pVR->IsConfigured())
				{
					pVR->DeleteClassifier(classifier_id,
						DELEGATE(ImageClassifier, OnDeleteClassifier, IService::Request *, this));
				}
			}
			else
				Log::Warning("ImageClassifier", "Unknown classifier status %s", status.c_str());
		}

		// if we didn't find our current classifier, then clear out the data..
		if (!bClassifierFound)
		{
			m_ClassifierDate.clear();
			m_ClassifierId.clear();
		}

		if (m_nPendingOps == 0)
			InitializeClassifier();			// no classifiers found, try to create the classifier then..
	}
	else
	{
		Log::Error("ImageClassifier", "Failed to get classifiers.");
	}
}

void ImageClassifier::OnGetClassifier(const Json::Value & a_Response)
{
	IVisualRecognition * pVR = Config::Instance()->FindService<IVisualRecognition>(m_ServiceId);

	m_nPendingOps -= 1;
	if (!a_Response.isNull())
	{
		const std::string & id = a_Response["classifier_id"].asString();
		const std::string & date = a_Response["created"].asString();
		const std::string & status = a_Response["status"].asString();

		Log::Status("ImageClassifier", "Checking classifier %s, date: %s, status: %s",
			id.c_str(), date.c_str(), status.c_str());
		if (status == "ready" && date.compare(m_ClassifierDate) >= 0)
		{
			if (m_ClassifierId.size() > 0)
			{
				if (pVR != NULL && pVR->IsConfigured())
				{
					Log::Status("ImageClassifier", "Deleting old classifier %s", m_ClassifierId.c_str());
					pVR->DeleteClassifier(m_ClassifierId,
						DELEGATE(ImageClassifier, OnDeleteClassifier, IService::Request *, this));
				}
			}

			m_ClassifierDate = date;
			m_ClassifierId = id;
		}
		else if (id != m_ClassifierId)
		{
			if (pVR != NULL && pVR->IsConfigured())
			{
				Log::Status("ImageClassifier", "Deleting old classifier %s", id.c_str());
				pVR->DeleteClassifier(id,
					DELEGATE(ImageClassifier, OnDeleteClassifier, IService::Request *, this));
			}
		}
	}

	if (m_nPendingOps == 0)
	{
		if (m_ClassifierId.size() > 0)
			Log::Status("ImageClassifier", "Selecting classifier %s, date: %s", m_ClassifierId.c_str(), m_ClassifierDate.c_str());
		else
			InitializeClassifier();		// no classifier got selected or is available, initialize a new one..
	}
}

void ImageClassifier::OnDeleteClassifier(IService::Request * a_Response)
{
	if (a_Response->IsError())
		Log::Status("ImageClassifier", "Failed to delete classifier");
	else
		Log::Status("ImageClassifier", "Classifier deleted.");
}

void ImageClassifier::SubmitPositiveImages(const std::list< Image::SP> & a_Images, const std::string & a_Class)
{
	if (a_Images.size() < 10)
		Log::Warning("ImageClassifier", "Submitting less than 10 images will not update trainer.");

	ZipFile::ZipMap zipData;
	for (std::list< Image::SP>::const_iterator i = a_Images.begin(); i != a_Images.end(); ++i)
		zipData[StringUtil::Format("%p.jpg", (*i).get())] = (*i)->GetContent();

	std::string zip;
	if (ZipFile::Deflate(zipData, zip))
	{
		std::string id(a_Class + "_positive_examples");
		m_ImagesCache.Save(id, zip, false);

		UpdateClassifier(id);
	}
	else
		Log::Error("ImageClassifer", "Failed to archive images.");
}

void ImageClassifier::SubmitNegativeImages(const std::list< Image::SP > & a_Images, const std::string & a_Class)
{
	if (a_Images.size() < 10)
		Log::Warning("ImageClassifier", "Submitting less than 10 images will not update trainer.");

	ZipFile::ZipMap zipData;
	for (std::list< Image::SP>::const_iterator i = a_Images.begin(); i != a_Images.end(); ++i)
		zipData[StringUtil::Format("%p.jpg", (*i).get())] = (*i)->GetContent();

	std::string zip;
	if (ZipFile::Deflate(zipData, zip))
	{
		std::string id(a_Class + "_negative_examples");
		m_ImagesCache.Save(id, zip, false);

		UpdateClassifier(id);
	}
	else
		Log::Error("ImageClassifer", "Failed to archive images.");
}

void ImageClassifier::OnHealth(const ThingEvent & a_Event)
{
	Health::SP spHealth = DynamicCast<Health>(a_Event.GetIThing());

	if (spHealth && spHealth->GetHealthName() == "Network" && spHealth->GetState() == "Timeouts")
	{
		Log::Status("ImageClassifier", "Stopping ImageClassifier due to network congestion");
		OnStop();

		// start timer to turn IVisualRecognition back on after some time
		m_spRestartTimer = TimerPool::Instance()->StartTimer(
			VOID_DELEGATE(ImageClassifier, RestartClassifier, this), m_fRestartTime, true, false);
	}
}

void ImageClassifier::RestartClassifier()
{
	Log::Status("ImageClassifier", "Restarting ImageClassifier.");
	OnStart();

	m_spRestartTimer.reset();
}

void ImageClassifier::UpdateClassifier(const std::string & a_ExampleId)
{
	if (m_ClassifierId.size() > 0)
	{
		m_PendingExamples.push_back(a_ExampleId);
		UpdateClassifier();
	}
	else
	{
		// no classifier just yet, try to create one ..
		InitializeClassifier();
	}
}

void ImageClassifier::InitializeClassifier()
{
	IVisualRecognition * pVR = Config::Instance()->FindService<IVisualRecognition>(m_ServiceId);
	if (pVR != NULL && pVR->IsConfigured())
	{
		const DataCache::CacheItemMap & cache = m_ImagesCache.GetCacheMap();
		if (cache.size() >= 2)
		{
			Log::Status("ImageClassifier", "Creating new classifier %s", m_ClassifierId.c_str());

			std::vector<std::string> negatives;
			std::vector<std::string> positives;
			for (DataCache::CacheItemMap::const_iterator iItem = cache.begin();
				iItem != cache.end(); ++iItem)
			{
				const DataCache::CacheItem & item = iItem->second;
				if (StringUtil::EndsWith(item.m_Id, "_positive_examples"))
					positives.push_back(item.m_Path);
				else if (StringUtil::EndsWith(item.m_Id, "_negative_examples"))
					negatives.push_back(item.m_Path);
			}

			pVR->CreateClassifier(m_ClassifierName,
				positives,
				negatives.size() > 0 ? negatives[0] : EMPTY_STRING,
				DELEGATE(ImageClassifier, OnCreateClassifier, const Json::Value &, this));
			m_PendingExamples.clear();
			m_bForceRetrain = false;

		}
		else
		{
			// we need some data to initialize the classifier with first..
			Log::Warning("ImageClassifier", "Not enough data to train the image classifier, please train with images.");
		}
	}
}

void ImageClassifier::OnCreateClassifier(const Json::Value & a_Response)
{
	if (!a_Response.isNull())
	{
		Log::Status("ImageClassifier", "Created custom classifier %s", a_Response.toStyledString().c_str());
		m_ClassifierId = a_Response["classifier_id"].asString();

		CheckTrainingClassifier();
	}
	else
		Log::Error("ImageClassifier", "Failed to create classifier %s", m_ClassifierId.c_str());
}

void ImageClassifier::UpdateClassifier()
{
	IVisualRecognition * pVR = Config::Instance()->FindService<IVisualRecognition>(m_ServiceId);
	if (pVR != NULL && pVR->IsConfigured())
	{
		assert(m_ClassifierId.size() > 0);

		// if we are already training, then nothing to do ATM, once training completes it will recall
		// this function..
		if (m_PendingExamples.begin() != m_PendingExamples.end() && !m_spRetrainingTimer)
		{
			std::string negative;
			std::vector<std::string> positives;
			for (FileList::iterator iItem = m_PendingExamples.begin();
				iItem != m_PendingExamples.end(); )
			{
				const std::string & id = *iItem;

				DataCache::CacheItem * pItem = m_ImagesCache.Find(id, false);
				if (pItem == NULL)
				{
					m_PendingExamples.erase(iItem++);
					continue;
				}

				if (StringUtil::EndsWith(id, "_positive_examples"))
				{
					positives.push_back(pItem->m_Path);
					m_PendingExamples.erase(iItem++);
				}
				else if (negative.size() == 0)
				{
					negative = pItem->m_Path;
					m_PendingExamples.erase(iItem++);
				}
				else
					++iItem;			// we already have a negative, skip this..
			}

			// if we only have a negative example to update with, then we need to go look in the cache
			// and a positive example of the same class if possible, otherwise any other positive example
			// will be used.
			if (positives.size() == 0 && negative.size() > 0)
			{
				std::string positive_id(negative);
				StringUtil::Replace(positive_id, "_negative_examples.zip", "_positive_examples");

				const DataCache::CacheItem * pItem = m_ImagesCache.Find(positive_id, false);
				if (pItem == NULL)
				{
					// no positive example of our class was found, just use the first positive example in the cache then..
					const DataCache::CacheItemMap & cache = m_ImagesCache.GetCacheMap();
					for (DataCache::CacheItemMap::const_iterator iItem = cache.begin(); iItem != cache.end(); ++iItem)
					{
						if (StringUtil::EndsWith(iItem->second.m_Id, "_positive_examples"))
						{
							positives.push_back(iItem->second.m_Path);
							break;
						}
					}
				}
				else
				{
					// YAY...  found positive example for our negative example.. use it.
					positives.push_back(pItem->m_Path);
				}
			}

			if (positives.size() >= 1)
			{
				Log::Status("ImageClassifier", "Updating classifier %s with %u positive examples, and %s",
					m_ClassifierId.c_str(), positives.size(),
					negative.size() > 0 ? "a negative example" : "no negative examples");

				pVR->UpdateClassifier(m_ClassifierId,
					m_ClassifierName,
					positives,
					negative,
					DELEGATE(ImageClassifier, OnUpdateClassifier, const Json::Value &, this));
			}
			else
			{
				Log::Warning("ImageClassifier", "Cannot update classifier yet, not enough data.");
				if (negative.size() > 0)
					m_PendingExamples.push_back(negative);
			}
		}
	}
}

void ImageClassifier::OnUpdateClassifier(const Json::Value & a_Response)
{
	if (a_Response.isNull())
	{
		Log::Error("ImageClassifier", "Failed to update the classifier, training new classifier.");
		m_spRetrainingTimer.reset();
		m_ClassifierId.clear();

		InitializeClassifier();
	}
	else
	{
		Log::Status("ImageClassifier", "Classifier is retraining");
		CheckTrainingClassifier();
	}
}

void ImageClassifier::CheckTrainingClassifier()
{
	IVisualRecognition * pVR = Config::Instance()->FindService<IVisualRecognition>(m_ServiceId);
	if (pVR != NULL && pVR->IsConfigured())
	{
		pVR->GetClassifier(m_ClassifierId,
			DELEGATE(ImageClassifier, OnCheckClassifier, const Json::Value &, this));

		if (!m_spRetrainingTimer)
		{
			m_spRetrainingTimer = TimerPool::Instance()->StartTimer(
				VOID_DELEGATE(ImageClassifier, CheckTrainingClassifier, this), CHECK_RETRAINING_INVERVAL, true, true);
		}
	}
}

void ImageClassifier::OnCheckClassifier(const Json::Value & a_Response)
{
	if (a_Response.isMember("status"))
	{
		const std::string & status = a_Response["status"].asString();

		Log::Status("ImageClassifier", "Training classifier status: %s", status.c_str());
		if (status == "ready")
		{
			m_spRetrainingTimer.reset();

			// do we have no examples to train, if so start updating the classifier again..
			if (m_PendingExamples.begin() != m_PendingExamples.end())
				UpdateClassifier();
		}
	}
}

