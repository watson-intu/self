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


#ifndef SELF_VERTEX_H
#define SELF_VERTEX_H

#include "models/IVertex.h"

class SELF_API SelfVertex : public IVertex
{
public:
	RTTI_DECL();

	typedef boost::shared_ptr<SelfVertex>		SP;
	typedef boost::weak_ptr<SelfVertex>			WP;

	SelfVertex() : m_bDropped( false ), m_bCreated( false ), m_bUpdated( false ), m_nRetries( 0 )
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IVertex interface
	virtual EdgeSP		CreateEdge( const std::string & a_Label,			//!< add a edge from this vertex to the given vertex
							const IVertex::SP & a_Dst,
							const PropertyMap & a_Properties );				
	virtual bool		Save();												//!< save changes to this vertex back into the data store
	virtual bool		Drop();												//!< remove this vertex

	SP shared_from_this()
	{
		return boost::static_pointer_cast<SelfVertex>( IVertex::shared_from_this() );
	}

protected:
	//! Data
	SP					m_spThis;
	bool				m_bDropped;
	bool				m_bCreated;
	bool				m_bUpdated;
	int					m_nRetries;

	//! Callbacks
	void				OnVertexCreated( const Json::Value & a_Result );
	void				OnVertexUpdated( const Json::Value & a_Result );
	void				OnVertexDeleted( const Json::Value & a_Result );
	void				SaveLocal();
	void				DeleteLocal();

	friend class SelfGraph;
	friend class SelfEdge;
	friend class ISelfTraverser;
};

#endif
