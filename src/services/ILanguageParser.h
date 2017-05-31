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


#ifndef SELF_ILANGUAGEPARSER_H
#define SELF_ILANGUAGEPARSER_H

#include "utils/IService.h"
#include "SelfLib.h"			// include last always

//! Wrapper for language parsing service
class SELF_API ILanguageParser : public IService
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<ILanguageParser>			SP;
	typedef boost::weak_ptr<ILanguageParser>			WP;

	ILanguageParser( const std::string & a_ServiceId, AuthType a_AuthType = AUTH_BASIC ) : 
		IService( a_ServiceId, a_AuthType )
	{}

	//! Get the parts of speech for the given text
	virtual void GetPosTags(const std::string & a_Text,
		Delegate<const Json::Value &> a_Callback ) = 0;
	//! Get the entities of the given text
	virtual void GetEntities(const std::string & a_Text,
		Delegate<const Json::Value &> a_Callback) = 0;

	//! Helper function to extract the city name from the entities
	static bool FindCity(const Json::Value & a_Entities,
		std::string & a_City);
};

#endif //SELF_ILANGUAGEPARSER_H
