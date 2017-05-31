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


#include "IPackageStore.h"
#include "utils/StringUtil.h"

RTTI_IMPL(IPackageStore, IService);

int IPackageStore::VersionCompare(const std::string & a_FirstVersion, 
	const std::string & a_SecondVersion)
{
	if (StringUtil::Compare(a_FirstVersion, a_SecondVersion, false) == 0)
		return 0;

	std::vector<std::string> firstVersionParts;
	std::vector<std::string> secondVersionParts;

	StringUtil::Split(a_FirstVersion, ".", firstVersionParts);
	StringUtil::Split(a_SecondVersion, ".", secondVersionParts);

	if (firstVersionParts.size() != secondVersionParts.size() )
		Log::Warning("IPackageStore", "Mismatched version sizes!");

	for (size_t x = 0; x < firstVersionParts.size(); x++)
	{
		if (std::atoi(firstVersionParts[x].c_str()) > std::atoi(secondVersionParts[x].c_str()))
		{
			return 1;
		}
		else if (std::atoi(firstVersionParts[x].c_str()) < std::atoi(secondVersionParts[x].c_str()))
		{
			return -1;
		}
	}

	return 0;
}

