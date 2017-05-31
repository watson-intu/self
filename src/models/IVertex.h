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


#ifndef SELF_IVERTEX_H
#define SELF_IVERTEX_H

#include "utils/ISerializable.h"
#include "utils/WatsonException.h"
#include "utils/JsonHelpers.h"
#include "utils/UniqueID.h"
#include "utils/Delegate.h"

#include "SelfLib.h"

class IEdge;			// forward declare
class IGraph;

//! Locally created ID's are prefixed with this character so we know if we
//! are working with an ID from the cloud or one generated locally that is considered
//! temporary.
#define LOCAL_ID_CHAR		'!'

//! This class defines a vertices's in the knowledge graph. 
class SELF_API IVertex : public ISerializable, 
	public boost::enable_shared_from_this<IVertex>
{
public:
	RTTI_DECL();

	//!Types
	typedef boost::shared_ptr<IVertex>			SP;
	typedef boost::weak_ptr<IVertex>			WP;
	typedef boost::shared_ptr<IEdge>			EdgeSP;
	typedef std::vector< EdgeSP >				EdgeList;
	typedef std::string							VertexId;
	typedef Json::Value							PropertyMap;

	enum VertexEvents {
		E_COMMITTED,				//!< vertex is committed into the graph
		E_MODIFIED,				//!< properties of the vertex are changed
		E_DROPPING				//!< vertex is being dropped, this is called before any data is destroyed
	};
	struct VertexEvent {
		VertexEvent( VertexEvents a_Type, const SP & a_spVertex ) 
			: m_Type( a_Type ), m_spVertex( a_spVertex )
		{}

		VertexEvents		m_Type;
		SP					m_spVertex;
	};
	typedef DelegateList<const VertexEvent &>	NotificationList;

	IVertex() : m_pGraph( NULL ), m_Id( LOCAL_ID_CHAR + UniqueID().Get() ), m_fTime( 0.0 )
	{}
	virtual ~IVertex()
	{}

	//! Interface
	virtual EdgeSP		CreateEdge( const std::string & a_Label,				//!< add a edge from this vertex to the given vertex
							const IVertex::SP & a_Dst,
							const PropertyMap & a_Properties ) = 0;				
	virtual bool		Save() = 0;												//!< save changes to this vertex back into the data store
	virtual bool		Drop() = 0;												//!< remove this vertex

	//! Accessors
	IGraph *			GetGraph() const;
	const VertexId &	GetId() const;
	const std::string &	GetLabel() const;
	const Json::Value & GetProperty(const std::string & a_Property) const;
	const Json::Value & operator[](const std::string & a_Property) const;
	const PropertyMap & GetProperties() const;
	double				GetTime() const;
	const EdgeList &	GetInEdges() const;
	const EdgeList &	GetOutEdges() const;

	//! Mutators
	Json::Value &		GetProperty(const std::string & a_Property);
	Json::Value &		operator[](const std::string & a_Property);
	PropertyMap &		GetProperties();
	void				SetProperties(const PropertyMap & a_Properties );

	NotificationList &	GetNotificationList();									//!< Get the notification list for this vertex, this is invoked when this object modified

	//! Find all out edges with the given label, edges will be sorted by weight (highest first) and time
	bool				FindOutEdges( const std::string & a_EdgeLabel, EdgeList & a_Edges );
	//! Find all in edges with the given label, edges will be sorted by weight (highest first) and time
	bool				FindInEdges( const std::string & a_EdgeLabel, EdgeList & a_Edges );
	//! Helper function to sort edges by weight/time
	static bool			SortEdges( const EdgeSP & a1, const EdgeSP & a2 );

	//! Helper function to add an edge with no properties
	EdgeSP				CreateEdge( const std::string & a_Label,
							const IVertex::SP & a_Dst )
	{
		return CreateEdge( a_Label, a_Dst, PropertyMap() );
	}
	//! Create a bi-direction connections with another vertex 
	void				Connect( const std::string & a_Label,
							const IVertex::SP & a_Dst )
	{
		CreateEdge( a_Label, a_Dst );
		a_Dst->CreateEdge( a_Label, shared_from_this() );
	}

protected:
	//! Data
	VertexId			m_Id;				// Id of this vertex in the graph
	std::string			m_Label;			// vertex label
	PropertyMap			m_Properties;		// properties of this vertex
	double				m_fTime;			// last time this vertex was touched

	EdgeList			m_InEdges;
	EdgeList			m_OutEdges;
	IGraph *			m_pGraph;			// the graph this vertex belongs
	NotificationList	m_NotificationList;

	void				SetGraph(IGraph * a_pGraph);
	void				SetId( const VertexId & a_nIndex);
	void				SetLabel(const std::string & a_label );
	void				SetTime(double a_fTime);

	void				AddInEdge(const EdgeSP & a_spEdge);
	bool				RemoveInEdge(const EdgeSP & a_spEdge);
	void				AddOutEdge(const EdgeSP & a_spEdge);
	bool				RemoveOutEdge(const EdgeSP & a_spEdge);

	friend class IGraph;
	friend class IEdge;
};

inline IGraph * IVertex::GetGraph() const
{
	return m_pGraph;
}

inline const IVertex::VertexId & IVertex::GetId() const
{
	return m_Id;
}

inline const std::string & IVertex::GetLabel() const
{
	return m_Label;
}

inline const Json::Value & IVertex::GetProperty(const std::string & a_Property) const
{
	return m_Properties[ a_Property ];
}

inline const Json::Value & IVertex::operator[]( const std::string & a_Property ) const
{
	return m_Properties[ a_Property ];
}

inline const IVertex::PropertyMap & IVertex::GetProperties() const
{
	return m_Properties;
}

inline double IVertex::GetTime() const
{
	return m_fTime;
}

inline const IVertex::EdgeList & IVertex::GetInEdges() const
{
	return m_InEdges;
}

inline const IVertex::EdgeList & IVertex::GetOutEdges() const
{
	return m_OutEdges;
}

inline void IVertex::SetGraph(IGraph * a_pGraph)
{
	m_pGraph = a_pGraph;
}

inline void IVertex::SetId( const VertexId & a_Id )
{
	m_Id = a_Id;
	m_NotificationList.Invoke( VertexEvent( E_COMMITTED, shared_from_this() ) );
}

inline void IVertex::SetLabel(const std::string & a_label )
{
	m_Label = a_label;
	m_Properties["_label"] = a_label;		// keep stored in properties as well
}

inline Json::Value & IVertex::GetProperty(const std::string & a_Property) 
{
	return m_Properties[ a_Property ];
}

inline Json::Value & IVertex::operator[]( const std::string & a_Property )
{
	return m_Properties[ a_Property ];
}

inline IVertex::PropertyMap & IVertex::GetProperties()
{
	return m_Properties;
}

inline void IVertex::SetProperties( const PropertyMap & a_Properties )
{
	m_Properties = a_Properties;
	m_Properties["_label"] = m_Label;
	m_NotificationList.Invoke( VertexEvent(E_MODIFIED,shared_from_this()) );
}

inline IVertex::NotificationList & IVertex::GetNotificationList() 
{
	return m_NotificationList;
}

inline void IVertex::SetTime(double a_fTime)
{
	m_fTime = a_fTime;
}

inline void IVertex::AddInEdge( const EdgeSP & a_spEdge )
{
	m_InEdges.push_back( a_spEdge );
}

inline bool IVertex::RemoveInEdge( const EdgeSP & a_spEdge )
{
	for(size_t i=0;i<m_InEdges.size();++i)
	{
		if ( m_InEdges[i] == a_spEdge )
		{
			m_InEdges.erase( m_InEdges.begin() + i );
			return true;
		}
	}

	return false;
}

inline void IVertex::AddOutEdge( const EdgeSP & a_spEdge )
{
	m_OutEdges.push_back( a_spEdge );
}

inline bool IVertex::RemoveOutEdge( const EdgeSP & a_spEdge )
{
	for(size_t i=0;i<m_OutEdges.size();++i)
	{
		if ( m_OutEdges[i] == a_spEdge )
		{
			m_OutEdges.erase( m_OutEdges.begin() + i );
			return true;
		}
	}

	return false;
}

#endif
