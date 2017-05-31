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


#ifndef SELF_IURL_H
#define SELF_IURL_H

#include "utils/IService.h"
#include "blackboard/URL.h"
#include "SelfLib.h"			// include last always

//! This interface is invoked to display a URL from self.
class SELF_API IBrowser : public IService
{
public:
	RTTI_DECL();

	//! Types
	struct URLServiceData
	{
		Json::Value             m_JsonValue;
		Url::SP                 m_spUrl;
	};

	//! Typedefs
	typedef Delegate<URLServiceData *>		        UrlCallback;
	typedef boost::shared_ptr<IBrowser>					SP;
	typedef boost::weak_ptr<IBrowser>					WP;

	IBrowser() : IService( "IBrowserV1", AUTH_NONE )
	{}
	IBrowser( const std::string & a_ServiceId, AuthType a_AuthType = AUTH_BASIC ) : 
		IService( a_ServiceId, a_AuthType )
	{}

	//! Display the given URL
	virtual void ShowURL(const Url::SP & a_spUrlAgent, UrlCallback a_Callback) = 0;

	//! Escape the given URL of any spaces found in any of the arguments, this is used to convert
	//! some text into a fully escaped URL.
	static std::string EscapeUrl( const std::string & a_URL );
};

#endif //SELF_IAVATAR_H
