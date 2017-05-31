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


#include "ILanguageParser.h"

RTTI_IMPL( ILanguageParser, IService );

bool ILanguageParser::FindCity(const Json::Value & a_Entities, std::string & a_City)
{
	if( a_Entities.isMember("entities") )
	{
		std::string location;
		for(size_t i=0;i<a_Entities["entities"].size();++i)
		{
			if (!a_City.empty())
				break;

			// Only continue if the entity is a Location
			if (a_Entities["entities"][i]["type"].asString() != "Location")
				continue;

			location = a_Entities["entities"][i]["text"].asString();
			if (a_Entities["entities"][i].isMember("disambiguation") &&
				a_Entities["entities"][i]["disambiguation"].isMember("subtype"))
			{
				Json::Value subtypeList = a_Entities["entities"][i]["disambiguation"]["subtype"];
				for(size_t j=0;j<subtypeList.size();++j)
				{
					// NLU's subtype list contains City
					if (subtypeList[j].asString() == "City")
					{
						a_City = a_Entities["entities"][i]["text"].asString();
						break;
					}
				}
			}
		}

		// If City entity was not detected but Location entity was
		if (!location.empty() && a_City.empty())
			a_City = location;
	}

	if (a_City.empty())
		return false;
	else
		return true;
}
