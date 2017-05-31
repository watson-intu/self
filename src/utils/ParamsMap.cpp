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


#include "ParamsMap.h"
#include "utils/StringUtil.h"

REG_SERIALIZABLE( ParamsMap );
RTTI_IMPL( ParamsMap, ISerializable );

void ParamsMap::Serialize(Json::Value & json)
{
	json = m_Data;
}

void ParamsMap::Deserialize(const Json::Value & json)
{
	m_Data = json;
	m_Data["Type_"] = "ParamsMap";		// make sure we add the Type_ entry..
}

std::string ParamsMap::ResolveVariables(const std::string & a_Input)
{
	std::string result = a_Input;

	size_t start = result.find("${");
	while (start != std::string::npos)
	{
		size_t end = result.find("}", start);
		if (end == std::string::npos)
			break;

		size_t len = (end - start) + 1;
		std::string var = result.substr(start + 2, len - 3);
		if (ValidPath(var))
		{
			const Json::Value & value = operator[](var);

			std::string data;
			if (! value.isObject() && !value.isArray() )
				data = value.asString();
			else
				data = value.toStyledString();

			result.replace( start, len, data );
			end = start + data.size();
		}
		else
			result.replace( start, len, "" );

		start = result.find("${", end );
	}

	return result;
}

Json::Value ParamsMap::ResolveVariables(const Json::Value & a_Json)
{
	if (a_Json.isArray())
	{
		Json::Value array;
		for (size_t i = 0; i < a_Json.size(); ++i)
			array[i] = ResolveVariables(a_Json[i]);
		return array;
	}
	else if (a_Json.isObject())
	{
		Json::Value object;
		for (Json::ValueConstIterator iElement = a_Json.begin(); iElement != a_Json.end(); ++iElement)
			object[ResolveVariables(iElement.key().asString())] = ResolveVariables(*iElement);
		return object;
	}

	return Json::Value(ResolveVariables(a_Json.asString()));
}

