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


#ifndef SELF_PROXY_EXTRACTOR_H
#define SELF_PROXY_EXTRACTOR_H

#include "IExtractor.h"
#include "SelfLib.h"

//! This extractor class is used to represent a remote extractor that is running outside this self instance
class SELF_API ProxyExtractor : public IExtractor
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr< ProxyExtractor >		SP;
	typedef boost::weak_ptr< ProxyExtractor >		WP;

	//! Construction
	ProxyExtractor(const std::string & a_ExtractorName,
		const std::string & a_InstanceId,
		bool a_bOverride,
		const std::string & a_Origin);
	ProxyExtractor();
	~ProxyExtractor();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IFeatureExtractor interface	
	virtual bool OnStart();
	virtual bool OnStop();
	virtual const char * GetName() const;

	void SendEvent(const std::string & a_EventName);

	const std::string & GetOrigin() const
	{
		return m_Origin;
	}

protected:
	//! Data
	std::string			m_Origin;
	std::string			m_ExtractorName;
	bool				m_bOverride;
};

#endif // SELF_PROXY_EXTRACTOR_H
