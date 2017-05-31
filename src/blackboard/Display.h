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


#ifndef SELF_DISPLAY_H
#define SELF_DISPLAY_H

#include "IThing.h"

//! This object is placed on the blackboard when we need self to say anything
class SELF_API Display : public IThing
{
public:
    RTTI_DECL();

    //! Types
    typedef boost::shared_ptr<Display>			SP;
    typedef boost::weak_ptr<Display>			WP;

    //! ISerializable interface
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

    //! Construction
    Display() : IThing( TT_COGNITIVE )
    {}

    Display(const std::string & a_DisplayType,
            const std::string & a_Data) :
            IThing( TT_COGNITIVE ),
            m_DisplayType ( a_DisplayType ),
            m_Data ( a_Data )
    {}

    //! Accessors
    const std::string & GetDisplay() const
    {
        return m_DisplayType;
    }

    void SetDisplay( const std::string & a_DisplayType )
    {
        m_DisplayType = a_DisplayType;
    }

    const std::string & GetData() const
    {
        return m_Data;
    }

    void SetData( const std::string & a_Data )
    {
        m_Data = a_Data;
    }

private:
    //! Data
    std::string m_DisplayType;
    std::string m_Data;
};

#endif //SELF_IMAGE_H
