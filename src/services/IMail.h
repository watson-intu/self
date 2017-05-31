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


#ifndef SELF_IMAIL_H
#define SELF_IMAIL_H

#include "utils/IService.h"
#include "SelfLib.h"			// include last always

//! This service interface provides a common service interface so self can send an email.
class SELF_API IMail : public IService
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<IMail>			SP;
	typedef boost::weak_ptr<IMail>				WP;

	IMail( const std::string & a_ServiceId, AuthType a_AuthType = AUTH_BASIC ) : 
		IService( a_ServiceId, a_AuthType )
	{}

	//! Send e-mail to gateway
	virtual void SendEmail( const std::string & a_To,
		const std::string & a_Subject, 
		const std::string & a_Message, Delegate<Request *> a_Callback) = 0;
};

#endif //SELF_IMAIL_H
