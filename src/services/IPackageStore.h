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


#ifndef SELF_IPACKAGESTORE_H
#define SELF_IPACKAGESTORE_H

#include "utils/IService.h"
#include "SelfLib.h"			// include last always

class DialogContainer;

//! This service wraps the PackageStore application.
class SELF_API IPackageStore : public IService
{
public:
	RTTI_DECL();

	//! Construction
	IPackageStore(const std::string & a_ServiceId, AuthType a_Auth = AUTH_BASIC ) : 
		IService( a_ServiceId, a_Auth )
	{}

	virtual void DownloadPackage(const std::string & a_PackageId, const std::string & a_Version, 
		Delegate<const std::string &> a_Callback) = 0;
	virtual void GetVersions(const std::string & a_PackageId, 
		Delegate<const Json::Value &> a_Callback) = 0;
	
	//! Compare two version strings
	static int VersionCompare(const std::string & a_FirstVersion,
		const std::string & a_SecondVersion);
};

#endif