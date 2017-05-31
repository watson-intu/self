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


#ifndef OBJECT_H
#define OBJECT_H

#include <list>

#include "IThing.h"
#include "SelfLib.h"		// include last always

class SELF_API Object : public IThing
{
public:
    RTTI_DECL();

	Object() : IThing( TT_PERCEPTION )
	{}

    const std::list<std::string> & GetObjectTypes() const
    {
        return m_ObjectTypeList;
    }
    void AddObjectType( const std::string & a_ObjectType )
    {
        m_ObjectTypeList.push_back(a_ObjectType);
    }

    //! ISerializable interface
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

private:
    //! Data
    std::list<std::string> m_ObjectTypeList;
};

#endif
