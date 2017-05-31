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


#include "NonsenseFilter.h"

REG_SERIALIZABLE(NonsenseFilter);
RTTI_IMPL(NonsenseFilter, IClassFilter);

NonsenseFilter::NonsenseFilter() :
	m_NonsenseClass("nonsense"),
	m_MinIgnoreNonsense(0.95f),
	m_DynamicNonsenseInterval(0.88f),
	m_DynamicNonsenseIntervalIncrement(0.015f),
	m_bActive(true)
{}

void NonsenseFilter::Serialize(Json::Value & json)
{
	json["m_Chatter"] = m_NonsenseClass;
	json["m_MinIgnoreNonsense"] = m_MinIgnoreNonsense;
	json["m_DynamicNonsenseInterval"] = m_DynamicNonsenseInterval;
	json["m_DynamicNonsenseIntervalIncrement"] = m_DynamicNonsenseIntervalIncrement;
}

void NonsenseFilter::Deserialize(const Json::Value & json)
{
	if (json.isMember("m_Chatter"))
		m_NonsenseClass = json["m_Chatter"].asString();
	if (json.isMember("m_MinIgnoreNonsense"))
		m_MinIgnoreNonsense = json["m_MinIgnoreNonsense"].asFloat();
	if (json.isMember("m_DynamicNonsenseInterval"))
		m_DynamicNonsenseInterval = json["m_DynamicNonsenseInterval"].asFloat();
	if (json.isMember("m_DynamicNonsenseIntervalIncrement"))
		m_DynamicNonsenseIntervalIncrement = json["m_DynamicNonsenseIntervalIncrement"].asFloat();
}

bool NonsenseFilter::ApplyFilter(Json::Value & a_Intent)
{
	//Log::Debug("NonsenseFilter", "this is what we have to work with: %s", a_Intent.toStyledString().c_str());
	//check to see if the intent is a hidden conf key
	Json::Value &classes = a_Intent["classes"];
	if (classes.size() >= 2)
	{
		if (classes[0]["class_name"].asString() == "key_up")
		{
			m_DynamicNonsenseInterval = 1;
			m_MinIgnoreNonsense = (float)((pow(2.0, m_DynamicNonsenseInterval)) - 1.0f);

			Log::Debug("TextClassifier", "spike conf, values above will be ignored %f", m_MinIgnoreNonsense);
		}

		else if (classes[0]["class_name"].asString() == "key_down") {
			m_DynamicNonsenseInterval = 0.2f;
			m_MinIgnoreNonsense = (float)((pow(2.0, m_DynamicNonsenseInterval)) - 1.0);

			Log::Debug("TextClassifier", "drop conf, values above will be ignored %f", m_MinIgnoreNonsense);
		}
	}

	// strip out key_up/key_down classes
	Json::Value removed;
	for (size_t i = 0; i < classes.size(); ++i)
	{
		const std::string &class_name = classes[i]["class_name"].asString();
		if (class_name == "key_up" || class_name == "key_down")
			classes.removeIndex(i--, &removed);
	}
	if (classes.size() > 0)
		a_Intent["top_class"] = classes[0]["class_name"].asString();

	//check to see if the utterance was classified as nonsense
	if (a_Intent["top_class"].asString() == m_NonsenseClass)
	{
		//I am using a exponential function to control the dynamic level of the confidence.
		//in the below line I am decrementing the x value which gets fed into the function
		m_DynamicNonsenseInterval = m_DynamicNonsenseInterval - 2 * m_DynamicNonsenseIntervalIncrement;

		//This acts as a floor to the exponential function to prevent values below .1 from being calculated
		//by extension this prevents negatives as well
		if (m_DynamicNonsenseInterval < 0.2f) {
			m_DynamicNonsenseInterval = 0.2f;
		}

		//recalculating the dynamic confidence for the chatter filter using the exponential function
		m_MinIgnoreNonsense = (float)((pow(2.0, m_DynamicNonsenseInterval)) - 1.0);

		//Log::Debug("TextClassifier", "decreased chatter conf by step = %f", m_MinIgnoreNonsense);

	}
	else 			//otherwise we assume the classification was not nonsense
	{

		//I am using a exponential function to control the dynamic level of the confidence.
		//in the below line I am incrementing the x value which gets fed into the function
		m_DynamicNonsenseInterval = m_DynamicNonsenseInterval + 2 * m_DynamicNonsenseIntervalIncrement;

		//This acts as a ceiling to the exponential function to prevent values above 1 from being calculate.
		if (m_DynamicNonsenseInterval > 1)
			m_DynamicNonsenseInterval = 1;

		//recalculating the dynamic confidence for the chatter filter using the exponential function
		m_MinIgnoreNonsense = (float)((pow(2.0, m_DynamicNonsenseInterval)) - 1.0f);

		//Log::Debug("TextClassifier", "increased chatter conf by step, min ignore nonsense = %f", m_MinIgnoreNonsense);
	}

	if (a_Intent["top_class"].asString() == m_NonsenseClass)
	{
		Log::Debug("TextClassifier", "this could be chatter; chatter conf = %.2f / %.2f",
			classes[0]["confidence"].asFloat(), m_MinIgnoreNonsense);
		if (classes[0]["confidence"].asFloat() < m_MinIgnoreNonsense || !m_bActive)
		{
			if (classes.size() >= 2)
			{
				Json::Value removed;
				classes.removeIndex(0, &removed);

				a_Intent["top_class"] = classes[0]["class_name"].asString();
			}
		}
	}

	if (m_bActive)
	{
		// return a true if our top_class is the nonsense class..
		return a_Intent["top_class"].asString() == m_NonsenseClass;
	}
	else
	{
		Log::Debug("TextClassifier", "Nonsense filter currently disabled...");
		return false; // filter not active
	}
}

void NonsenseFilter::SetActiveState(bool a_State)
{
	m_bActive = a_State;
}

bool NonsenseFilter::GetActiveState()
{
	return m_bActive;
}