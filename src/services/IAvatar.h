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


#ifndef SELF_IAVATAR_H
#define SELF_IAVATAR_H

#include "utils/IService.h"
#include "SelfLib.h"			// include last always

//! This interface wraps a visual state avatar for this self instance. This service
//! is invoked by the AvatarGesture.
class SELF_API IAvatar : public IService
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<IAvatar>			SP;
	typedef boost::weak_ptr<IAvatar>				WP;

	IAvatar( const std::string & a_ServiceId, AuthType a_AuthType = AUTH_BASIC ) : 
		IService( a_ServiceId, a_AuthType )
	{}

	//! Change the state of the avatar
	virtual bool ChangeState( const std::string & a_NewState ) = 0;
};

#endif //SELF_IAVATAR_H
