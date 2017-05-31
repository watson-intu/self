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


#ifndef SELF_LOCAL_SPEECH_TO_TEXT_H
#define SELF_LOCAL_SPEECH_TO_TEXT_H

#include <list>

#include "TextData.h"
#include "ISensor.h"
#include "SelfLib.h"

//! This is the base class for a speech to text sensor implementation, this is not a concrete implementation.
class SELF_API LocalSpeechToText : public ISensor
{
public:
    RTTI_DECL();

    //! Construction
    LocalSpeechToText() : ISensor( "LocalSpeechToText" )
    {}

    ////! ISerializable interface
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

    //! ISensor interface
    virtual const char * GetDataType()
    {
        return "TextData";
    }

    virtual bool OnStart();
    virtual bool OnStop();
    virtual void OnPause();
    virtual void OnResume();

protected:
    //! Data
    float			            m_MinConfidence;
    std::vector<std::string>    m_VocabularyList;
};

#endif
