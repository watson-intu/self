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


#ifndef SELF_TRAVERSER_H
#define SELF_TRAVERSER_H

#include "models/ITraverser.h"

//! base class for all traverser classes
class SELF_API ISelfTraverser : public ITraverser
{
public:
	RTTI_DECL();

	typedef IVertex::PropertyMap					PropertyMap;
	typedef ITraverser::Condition					Condition;
	typedef boost::shared_ptr<ISelfTraverser>		SP;
	typedef boost::weak_ptr<ISelfTraverser>			WP;

	ISelfTraverser() : m_bRemoteTraverse( false ), m_bRemoteDone( false ), m_bLocalTraverse( false )
	{}
	ISelfTraverser(const Condition & a_Condition, SP a_spNext = SP())
		: ITraverser(a_Condition, a_spNext), m_bRemoteTraverse( false ), m_bRemoteDone( false )
	{}

	//! ITraverser interface
	virtual bool			Export( Json::Value & a_Export );				
	virtual ITraverser::SP	Filter(const Condition & a_spCond);
	virtual ITraverser::SP	Out(const Condition & a_spCond /*= NULL_CONDITION*/);
	virtual ITraverser::SP	In(const Condition & a_spCond /*= NULL_CONDITION*/);
	virtual bool			Start(TraverseCallback a_Callback);

	SP shared_from_this()
	{
		return boost::static_pointer_cast<ISelfTraverser>( ITraverser::shared_from_this() );
	}

protected:
	//! Data
	SP				m_spThis;
	bool			m_bRemoteTraverse;
	bool			m_bRemoteDone;
	bool			m_bLocalTraverse;

	std::string		m_Query;			// our gremlin query
	Json::Value		m_Bindings;			// our bindings

	//! Static functions
	static const char * GetEqualityOp(Logic::EqualityOp a_Op);
	static const char * GetLogicalOp(Logic::LogicalOp a_LogOp);
	static std::string GremlinCondition( IConditional::SP a_spCondition, Json::Value & a_Bindings );

	//! Execute our traverse
	void ExecuteTraverse();
	bool SendGremlinQuery();

	//! Callbacks
	void OnNextDone( ITraverser::SP a_spTraverser );
	void OnGremlinQuery( const Json::Value & a_Result );

	//! Interface
	virtual void BuildGremlinQuery( std::string & a_Query, Json::Value & a_Bindings ) = 0;
	virtual void OnLocalTraverse() = 0;

	friend class SelfGraph;
	friend class FilterTraverser;
	friend class InTraverser;
	friend class OutTraverser;
};

class SELF_API FilterTraverser : public ISelfTraverser
{
public:
	RTTI_DECL();

	FilterTraverser()
	{}
	FilterTraverser(const Condition & a_Condition, SP a_spNext = SP())
		: ISelfTraverser(a_Condition, a_spNext)
	{}

protected:
	//! ISelfTraverser interface
	virtual void BuildGremlinQuery( std::string & a_Query, Json::Value & a_Bindings );
	virtual void OnLocalTraverse();
};
class SELF_API OutTraverser : public ISelfTraverser
{
public:
	RTTI_DECL();

	OutTraverser()
	{}
	OutTraverser(const Condition & a_Condition, SP a_spNext = SP())
		: ISelfTraverser(a_Condition, a_spNext)
	{}

protected:
	//! ISelfTraverser interface
	virtual void BuildGremlinQuery( std::string & a_Query, Json::Value & a_Bindings );
	virtual void OnLocalTraverse();
};
class SELF_API InTraverser : public ISelfTraverser
{
public:
	RTTI_DECL();

	InTraverser()
	{}
	InTraverser(const Condition & a_Condition, SP a_spNext = SP())
		: ISelfTraverser(a_Condition, a_spNext)
	{}

protected:
	//! ISelfTraverser interface
	virtual void BuildGremlinQuery( std::string & a_Query, Json::Value & a_Bindings );
	virtual void OnLocalTraverse();
};

#endif
