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


#ifndef SELF_PROXY_GESTURE_H
#define SELF_PROXY_GESTURE_H

#include "utils/TimerPool.h"
#include "IGesture.h"
#include "SelfLib.h"

//! This gesture acts as a proxy for a remote gesture to be executed via the topic system
class SELF_API ProxyGesture : public IGesture
{
public:
	RTTI_DECL();

	//! Types
	typedef std::vector< IGesture::SP >		OverrideList;
	typedef boost::shared_ptr<ProxyGesture>	SP;
	typedef boost::weak_ptr<ProxyGesture>	WP;

	//! Construction
	ProxyGesture( const std::string & a_GestureId, 
		const std::string & a_InstanceId,
		bool a_bOverride,
		const std::string & a_Origin );
	ProxyGesture();
	~ProxyGesture();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IGesture interface
	virtual bool Start();
	virtual bool Stop();
	virtual bool Execute(GestureDelegate a_Callback, const ParamsMap & a_Params);
	virtual bool Abort();

	const std::string & GetInstanceId() const
	{
		return m_InstanceId;
	}
	const std::string & GetOrigin() const
	{
		return m_Origin;
	}

	//! Invoked by the GestureManager when a execute_done event is received.
	void OnExecuteDone( bool a_bError );

private:
	//! Data
	std::string			m_Origin;
	std::string			m_InstanceId;
	bool				m_bOverride;
	OverrideList		m_Overrides;
	TimerPool::ITimer::SP
						m_spTimeoutTimer;

	void ExecuteGesture();
	void OnTimeout();
};

#endif //SELF_WAITGESTURE_H