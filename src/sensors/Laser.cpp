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


#include "Laser.h"

REG_SERIALIZABLE( Laser );
RTTI_IMPL( Laser, ISensor );

void Laser::Serialize(Json::Value & json)
{
    ISensor::Serialize( json );
}

void Laser::Deserialize(const Json::Value & json)
{
    ISensor::Deserialize( json );
}

bool Laser::OnStart()
{
    Log::Debug( "Laser", "OnStart() invoked." );
    return true;
}

bool Laser::OnStop()
{
    Log::Debug( "Laser", "OnStop() invoked." );
    return true;
}

void Laser::OnPause()
{}

void Laser::OnResume()
{}

