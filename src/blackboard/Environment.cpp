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


#include "Environment.h"

REG_SERIALIZABLE(Environment);
RTTI_IMPL( Environment, IThing );

bool Environment::Create(const Json::Value & json)
{
    if ( json.isMember( "body" ) 
		&& json["body"].isMember("devices") 
		&& json["body"]["devices"].size() > 0 
		&& json["body"]["devices"][0].isMember("dashboard_data") )
	{
        m_CarbonDioxide = json["body"]["devices"][0]["dashboard_data"]["CO2"].asInt();
        m_Humidity = json["body"]["devices"][0]["dashboard_data"]["Humidity"].asInt();
        m_Pressure = json["body"]["devices"][0]["dashboard_data"]["Pressure"].asFloat();
        m_Temperature = json["body"]["devices"][0]["dashboard_data"]["Temperature"].asFloat();
		return true;
    }

	Log::Warning( "Environment", "Unsupported remote device: %s", json.toStyledString().c_str() );
	return false;
}

void Environment::Serialize(Json::Value &json)
{
    IThing::Serialize( json );

	json["m_CarbonDioxide"] = m_CarbonDioxide;
	json["m_Humidity" ] = m_Humidity;
	json["m_Temperature" ] = m_Temperature;
	json["m_Pressure" ] = m_Pressure;
}

void Environment::Deserialize(const Json::Value &json)
{
    IThing::Deserialize( json );

	if ( json.isMember( "m_CarbonDioxide" ) )
		m_CarbonDioxide = json["m_CarbonDioxide"].asInt();
	if ( json.isMember( "m_Humidity" ) )
		m_Humidity = json["m_Humidity"].asInt();
	if ( json.isMember( "m_Temperature" ) )
		m_Temperature = json["m_Temperature"].asFloat();
	if ( json.isMember( "m_Pressure" ) )
		m_Pressure = json["m_Pressure"].asFloat();
}