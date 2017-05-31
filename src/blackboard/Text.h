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


#ifndef SELF_TEXT_H
#define SELF_TEXT_H

#include "IThing.h"
#include "utils/StringUtil.h"

//! Recognized speech or entered text, usually attached to a person object if they are known. This object
//! will send the get processed by the TextClassifier object and a intent object will be attached to it on the 
//! blackboard.

//! NOTE: Language is set the original language before it has been translated into the back-end language
//! The DialogAgent will use this to know what to translate the language back into when responding to the user.
class SELF_API Text : public IThing
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<Text>		SP;
	typedef boost::weak_ptr<Text>		WP;

	//! Construction
	Text() : IThing( TT_PERCEPTION ), 
		m_LocalDialog( false ), 
		m_ClassifyIntent( true ), 
		m_fConfidence( 1.0 ), 
		m_Language( "en-US" )
	{}
	Text( const std::string & a_Speech, double a_fConfidence, bool a_bLocalDialog, bool a_bClassifyIntent, const std::string & a_Language = "en-US" ) : 
		IThing( TT_PERCEPTION ),
		m_Text ( StringUtil::Trim(a_Speech) ),
		m_fConfidence( a_fConfidence ),
		m_LocalDialog ( a_bLocalDialog ),
		m_ClassifyIntent( a_bClassifyIntent ),
		m_Language( a_Language )
	{}

	const std::string & GetText() const
	{
		return m_Text;
	}
	void SetText(const std::string & a_Text)
	{
		m_Text = a_Text;
	}

	double GetConfidence() const
	{
		return m_fConfidence;
	}
	void SetConfidence( double a_fConfidence )
	{
		m_fConfidence = a_fConfidence;
	}

	bool IsLocalDialog() const
	{
		return m_LocalDialog;
	}
	bool ClassifyIntent() const 
	{
		return m_ClassifyIntent;
	}

	//! Returns the language of this text transcript.
	const std::string & GetLanguage() const
	{
		return m_Language;
	}
	//! Set the language of this text transcript
	void SetLanguage( const std::string & a_Language )
	{
		m_Language = a_Language;
	}

	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

private:
	//! Data
	std::string m_Text;
	double		m_fConfidence;
	bool 		m_LocalDialog;
	bool		m_ClassifyIntent;
	std::string m_Language;
};

#endif
