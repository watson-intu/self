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


#include "RemoteDevice.h"

REG_SERIALIZABLE( RemoteDevice );
RTTI_IMPL(RemoteDevice, ISensor);
RTTI_IMPL_EMBEDDED(RemoteDevice, Rest, ISerializable );

void RemoteDevice::Serialize(Json::Value & json)
{
	ISensor::Serialize( json );
	json["m_fPollInterval"] = m_fPollInterval;
    SerializeVector("m_Rests", m_Rests, json);
}

void RemoteDevice::Deserialize(const Json::Value & json)
{
	ISensor::Deserialize( json );
	if (json.isMember("m_fPollInterval"))
		m_fPollInterval = json["m_fPollInterval"].asFloat();
    DeserializeVector("m_Rests", json, m_Rests);
}

bool RemoteDevice::OnStart()
{
    Log::Debug("RemoteDevice", "Starting up Remote Device");
    m_spWaitTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(RemoteDevice, StreamingThread, this), m_fPollInterval, true, true);
    return true;
}

void RemoteDevice::StreamingThread()
{
	if (! m_bPaused )
		new DeviceRequest(m_Rests, this);
}

bool RemoteDevice::OnStop()
{
    m_spWaitTimer.reset();
    return true;
}

void RemoteDevice::SendingData( RemoteDeviceData * a_pData )
{
    SendData( a_pData );
}

void RemoteDevice::OnPause()
{
	m_bPaused = true;
}

void RemoteDevice::OnResume()
{
	m_bPaused = false;
}

//--------------------------------------

RemoteDevice::DeviceRequest::DeviceRequest()
{}

RemoteDevice::DeviceRequest::DeviceRequest(const std::vector<RemoteDevice::Rest> & Rests, RemoteDevice * a_pDevice) :
	m_pDevice(a_pDevice), m_Rests(Rests), m_Index(0)
{
	MakeRequest();
}

RemoteDevice::DeviceRequest::~DeviceRequest()
{}

void RemoteDevice::DeviceRequest::OnState(IWebClient * a_pConnector)
{
	if (a_pConnector->GetState() == IWebClient::CLOSED 
		|| a_pConnector->GetState() == IWebClient::DISCONNECTED)
	{
		if (m_Index < (m_Rests.size() - 1) )
		{
			m_Index += 1;

			Json::Value req;
			m_Rests[m_Index].Serialize(req);
			req = m_Param.ResolveVariables(req);
			m_Rests[m_Index].Deserialize(req);

			MakeRequest();
		}
		else
		{
			ThreadPool::Instance()->InvokeOnMain<RemoteDeviceData *>(
				DELEGATE(RemoteDevice, SendingData, RemoteDeviceData *, m_pDevice),
				new RemoteDeviceData(m_Param["response"][m_Index]));

			delete this;
		}
	}
}

void RemoteDevice::DeviceRequest::OnResponse(IWebClient::RequestData * a_pResponse)
{
	Json::Value root;
	Json::Reader reader;
	reader.parse(a_pResponse->m_Content, root);
	m_Param["response"][m_Index] = root;
}

void RemoteDevice::DeviceRequest::MakeRequest()
{
	m_spClient = IWebClient::Request(m_Rests[m_Index].m_URL + m_Rests[m_Index].m_Params,
		m_Rests[m_Index].m_Headers,
		m_Rests[m_Index].m_Type,
		m_Rests[m_Index].m_Body,
		DELEGATE(DeviceRequest, OnResponse, IWebClient::RequestData *, this),
		DELEGATE(DeviceRequest, OnState, IWebClient *, this));
}
