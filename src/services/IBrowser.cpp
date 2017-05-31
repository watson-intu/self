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


#include "IBrowser.h"

RTTI_IMPL( IBrowser, IService );

std::string IBrowser::EscapeUrl( const std::string & a_URL )
{
	std::string url( a_URL );

	size_t argStart = url.find_first_of('?');
	if ( argStart != std::string::npos )
	{
		std::vector<std::string> args;
		StringUtil::Split( url.substr( argStart + 1 ), "&", args );

		url.erase( argStart + 1 );
		for(size_t i=0;i<args.size();++i)
		{
			if (i > 0)
				url += "&";

			std::string & arg = args[i];

			size_t equal = arg.find_first_of('=');
			if ( equal != std::string::npos )
			{
				std::string key( arg.substr( 0, equal ) );
				std::string value( arg.substr( equal + 1 ) );
				if (! StringUtil::IsEscaped( value ) )
					value = StringUtil::UrlEscape( value );
				arg = key + "=" + value;
			}
			else if (! StringUtil::IsEscaped( arg ) )
				arg = StringUtil::UrlEscape( arg );

			url += arg;
		}
	}

	return url;
}

