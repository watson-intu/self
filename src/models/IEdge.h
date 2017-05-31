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


#ifndef SELF_IEDGE_H
#define SELF_IEDGE_H

#include "utils/Time.h"

#include "IVertex.h"

//! This object defines a relationship between two vertices in this graph, this is one direction.
class SELF_API IEdge : public ISerializable, 
	public boost::enable_shared_from_this<IEdge>
{
public:
	RTTI_DECL();

	//!Types
	typedef boost::shared_ptr<IEdge>			SP;
	typedef boost::weak_ptr<IEdge>				WP;
	typedef std::string							EdgeId;
	typedef IVertex::VertexId					VertexId;
	typedef IVertex::PropertyMap				PropertyMap;

	enum EdgeEvents {
		E_COMMITTED,			//!< edge is committed into the graph
		E_MODIFIED,				//!< edge has been modified
		E_DROPPING				//!< edge is being dropped
	};
	struct EdgeEvent {
		EdgeEvent( EdgeEvents a_Type, const SP & a_spEdge ) 
			: m_Type( a_Type ), m_spEdge( a_spEdge )
		{}

		EdgeEvents			m_Type;
		SP					m_spEdge;
	};
	typedef DelegateList<const EdgeEvent &>	NotificationList;

	IEdge() : m_pGraph( NULL ), m_Id( LOCAL_ID_CHAR + UniqueID().Get() ), m_fTime( 0.0 )
	{}
	virtual ~IEdge()
	{}

	//! Interface
	virtual bool		Save() = 0;								//!< push changed data in this edge to the data store
	virtual bool		Drop() = 0;								//!< remove this edge

	//! Accessors
	IGraph *			GetGraph() const;
	const EdgeId &		GetId() const;
	const std::string & GetLabel() const;
	const Json::Value & GetProperty(const std::string & a_Property) const;
	const Json::Value & operator[](const std::string & a_Property) const;
	const PropertyMap & GetProperties() const;
	double				GetTime() const;
	VertexId			GetSourceId() const;
	IVertex::SP			GetSource() const;
	VertexId			GetDestinationId() const;
	IVertex::SP			GetDestination() const;

	//! Mutators
	Json::Value &		GetProperty(const std::string & a_Property);
	Json::Value &		operator[](const std::string & a_Property);
	PropertyMap &		GetProperties();
	void				SetProperties(const PropertyMap & a_Properties );

	NotificationList &	GetNotificationList();					//!< Get the notification list for this vertex, this is invoked when this object modified

protected:
	//! Data
	IGraph *			m_pGraph;
	EdgeId				m_Id;
	std::string			m_Label;
	PropertyMap			m_Properties;
	double				m_fTime;
	VertexId			m_SourceId;
	VertexId			m_DestinationId;

	IVertex::WP			m_Source;
	IVertex::WP			m_Destination;

	NotificationList	m_NotificationList;

	void				SetGraph(IGraph * a_pGraph );
	void				SetId( const EdgeId & a_Id );
	void				SetLabel(const std::string & a_Label);
	void				SetTime( double a_fTime );
	void				SetSource(const IVertex::SP & a_spSource);
	void				SetDestination(const IVertex::SP & a_spDest);
	void				SetSourceId( const VertexId & a_Id );
	void				SetDestinationId( const VertexId & a_Id );
};

inline IGraph * IEdge::GetGraph() const
{
	return m_pGraph;
}

inline const IEdge::EdgeId & IEdge::GetId() const
{
	return m_Id;
}

inline const std::string & IEdge::GetLabel() const
{
	return m_Label;
}

inline const Json::Value & IEdge::GetProperty(const std::string & a_Property) const
{
	return m_Properties[ a_Property ];
}

inline const Json::Value & IEdge::operator[]( const std::string & a_Property ) const
{
	return m_Properties[ a_Property ];
}

inline const IEdge::PropertyMap & IEdge::GetProperties() const
{
	return m_Properties;
}

inline double IEdge::GetTime() const
{
	return m_fTime;
}

inline IEdge::VertexId IEdge::GetSourceId() const
{
	return m_SourceId;
}

inline IVertex::SP IEdge::GetSource() const
{
	return m_Source.lock();
}

inline IEdge::VertexId IEdge::GetDestinationId() const
{
	return m_DestinationId;
}

inline IVertex::SP IEdge::GetDestination() const
{
	return m_Destination.lock();
}

inline void	IEdge::SetGraph(IGraph * a_pGraph )
{
	m_pGraph = a_pGraph;
}

inline void IEdge::SetId(const EdgeId & a_Id)
{
	m_Id = a_Id;
	m_NotificationList.Invoke( EdgeEvent( E_COMMITTED, shared_from_this() ) );
}

inline Json::Value & IEdge::GetProperty(const std::string & a_Property) 
{
	return m_Properties[ a_Property ];
}

inline Json::Value & IEdge::operator[]( const std::string & a_Property ) 
{
	return m_Properties[ a_Property ];
}

inline IEdge::PropertyMap & IEdge::GetProperties()
{
	return m_Properties;
}

inline void IEdge::SetProperties( const PropertyMap & a_Properties )
{
	m_Properties = a_Properties;
	m_Properties["_label"] = m_Label;		// restore the label property
	m_NotificationList.Invoke( EdgeEvent( E_MODIFIED, shared_from_this() ) );
}

inline IEdge::NotificationList & IEdge::GetNotificationList()
{
	return m_NotificationList;
}

inline void IEdge::SetLabel(const std::string & a_Label)
{
	m_Label = a_Label;
	m_Properties["_label"] = a_Label;
}

inline void IEdge::SetTime( double a_fTime )
{
	m_fTime = a_fTime;
}

inline void IEdge::SetSource( const IVertex::SP & a_spSource )
{
	IVertex::SP spOldSource = m_Source.lock();
	if ( spOldSource )
		spOldSource->RemoveOutEdge( shared_from_this() );
	
	m_Source = a_spSource;
	m_SourceId = a_spSource ? a_spSource->GetId() : "";

	if ( a_spSource )
		a_spSource->AddOutEdge( shared_from_this() );
}

inline void IEdge::SetDestination( const IVertex::SP & a_spDest )
{
	IVertex::SP spOldDest = m_Destination.lock();
	if ( spOldDest )
		spOldDest->RemoveInEdge( shared_from_this() );

	m_Destination = a_spDest;
	m_DestinationId = a_spDest ? a_spDest->GetId() : 0;

	if ( a_spDest )
		a_spDest->AddInEdge( shared_from_this() );
}

inline void IEdge::SetSourceId( const VertexId & a_Id )
{
	m_SourceId = a_Id;
}

inline void IEdge::SetDestinationId( const VertexId & a_Id )
{
	m_DestinationId = a_Id;
}

#endif
