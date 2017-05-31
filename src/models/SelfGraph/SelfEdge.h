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


#ifndef SELF_EDGE_H
#define SELF_EDGE_H

#include "models/IEdge.h"

class SELF_API SelfEdge : public IEdge
{
public:
	RTTI_DECL();

	typedef boost::shared_ptr<SelfEdge>			SP;
	typedef boost::weak_ptr<SelfEdge>			WP;

	SelfEdge() : m_bDropped(false), m_bCreated(false), m_bUpdated(false), m_nRetries(0)
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IEdge interface
	virtual bool		Save();
	virtual bool		Drop();

	SP shared_from_this()
	{
		return boost::static_pointer_cast<SelfEdge>(IEdge::shared_from_this());
	}

	//! This can be invoked to force this edge to find it's source & destination vertexes
	//! Returns false if edge can't find source or destination in local cache
	bool ResolveEdge();

protected:
	//! Data
	bool				m_bDropped;
	bool				m_bCreated;
	bool				m_bUpdated;
	SP					m_spThis;
	int					m_nRetries;

	//! Callbacks
	void				OnEdgeCreated(const Json::Value & a_Result);
	void				OnEdgeUpdated(const Json::Value & a_Result);
	void				OnEdgeDeleted(const Json::Value & a_Result);
	void				SaveLocal();
	void				DeleteLocal();

	friend class SelfGraph;
	friend class SelfVertex;
	friend class ISelfTraverser;
};

#endif
