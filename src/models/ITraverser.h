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


#ifndef SELF_ITRAVERSER_H
#define SELF_ITRAVERSER_H

#include "utils/IConditional.h"
#include "utils/Delegate.h"

#include "IVertex.h"
#include "IEdge.h"

class IGraph;		// forward declarations

//! This object is created the traverse the graph and search for vertex/edge objects. These objects
//! are usually chained together to form a more complex query, this is done by linking each ITraverser object
//! to the next object in the chain to form the query.

//! This object holds it's results in a vector of vertex indexes into the original graph object.
class SELF_API ITraverser : public ISerializable, 
	public boost::enable_shared_from_this<ITraverser>
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<ITraverser>		SP;
	typedef boost::weak_ptr<ITraverser>			WP;
	typedef IConditional						Condition;
	typedef boost::shared_ptr<IGraph>			IGraphSP;
	typedef boost::weak_ptr<IGraph>				IGraphWP;
	typedef IVertex::PropertyMap				PropertyMap;

	typedef std::vector<IVertex::SP>			VertextList;
	typedef Delegate<SP>						TraverseCallback;

	//! Constants
	static const NullCondition NULL_CONDITION;

	//! Construction
	ITraverser() : 
		m_pGraph( NULL )
	{}
	ITraverser(const Condition & a_Condition, const SP & a_spNext) :
		m_pGraph( a_spNext ? a_spNext->m_pGraph : NULL),
		m_spNext(a_spNext),
		m_spCondition(a_Condition.Clone())
	{}
	virtual ~ITraverser()
	{}

	//! ISerializable interface
	virtual void	Serialize(Json::Value & json);
	virtual void	Deserialize(const Json::Value & json);

	//! Accessors
	IGraph *			GetGraph() const;						//!< returns the owning graph
	SP					GetNext() const;						//!< returns the next ITraverser object

	const VertextList & GetResults() const;						//!< The Results of this traverser
	size_t				Size() const;							//!< Returns the size of the results
	IVertex::SP			GetResult( size_t i ) const;			//!< Get a result by index
	IVertex::SP 		operator[](size_t n) const;				//!< Help function to access the results

	//! Interface
	virtual bool		Export( Json::Value & a_Export ) = 0;	//!< Export this traverser into JSON
	virtual SP			Filter(const Condition & a_spCond) = 0;	//!< Get all vertexes that meet the provided conditions
	virtual SP			Out(const Condition & a_Condition = NULL_CONDITION) = 0;	//!< Get all vertexes with an out edge that meets the provided conditions
	virtual SP			In(const Condition & a_spCond = NULL_CONDITION) = 0;		//!< Get all vertexes with an in edge that meets the provided conditions

	//! Traverse the graph and invoke the provided callback once the search is completed. Since traverse
	//! make take some time to complete, this call is asynchronous.
	virtual bool	Start(TraverseCallback a_Callback ) = 0;

protected:
	//! Data
	IGraph *		m_pGraph;						// pointer to the owning graph object
	VertextList		m_Results;						// vector of the results

	SP				m_spNext;
	Condition::SP	m_spCondition;
	TraverseCallback m_Callback;

	//! Mutators
	void			SetGraph( IGraph * a_pGraph);
};

inline IGraph * ITraverser::GetGraph() const
{
	return m_pGraph;
}

inline ITraverser::SP ITraverser::GetNext() const
{
	return m_spNext;
}

inline const ITraverser::VertextList & ITraverser::GetResults() const
{
	return m_Results;
}

inline size_t ITraverser::Size() const
{
	return m_Results.size();
}

inline IVertex::SP ITraverser::GetResult( size_t i ) const
{
	if ( i < m_Results.size() )
		return m_Results[ i ];
	return IVertex::SP();
}

inline IVertex::SP ITraverser::operator [](size_t i ) const
{
	return GetResult( i );
}

inline void ITraverser::SetGraph( IGraph * a_pGraph)
{
	m_pGraph = a_pGraph;
	if ( m_spNext )
		m_spNext->SetGraph( m_pGraph );
}

#endif
