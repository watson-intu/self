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


#include "DataStoreSQLL.h"
#include "utils/Log.h"
#include "utils/Path.h"
#include "utils/JsonHelpers.h"
#include "utils/ThreadPool.h"
#include "utils/Config.h"
#include "sqlite/sqlite3.h"

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

namespace fs = boost::filesystem;

const std::string DB_DIRECTORY( "db/" );

RTTI_IMPL( DataStoreSQLL, IDataStore );

DataStoreSQLL::DataStoreSQLL() : m_pDB( NULL ), m_bStop( false ), m_bThreadStopped( true )
{}

DataStoreSQLL::~DataStoreSQLL()
{
	Stop();
}

bool DataStoreSQLL::Start( const std::string & a_DB, const Json::Value & a_DataDefinition )
{
	ThreadPool * pTP = ThreadPool::Instance();
	if ( pTP == NULL )
	{
		Log::Error( "DataStoreSQLL", "ThreadPool instance is NULL." );
		return false;
	}

	if (m_pDB != NULL )
		return false;

	std::string instanceData( "./" );

	Config * pConfig = Config::Instance();
	if ( pConfig != NULL )
		instanceData = pConfig->GetInstanceDataPath();

	std::string dbDirectory( instanceData + DB_DIRECTORY );

	m_DBFile = dbDirectory + Path( a_DB ).GetFile() + ".db";
	m_Definition = a_DataDefinition;
	m_bStop = false;

	// make the directory if needed..
	if (!fs::is_directory(dbDirectory))
	{
		try {
			fs::create_directories(dbDirectory);
		}
		catch (const std::exception & ex)
		{
			Log::Error("DataCache", "Caught Exception: %s", ex.what());
			return false;
		}
	}

	int rc = sqlite3_open(m_DBFile.c_str(), &m_pDB);
	if ( rc != SQLITE_OK )
	{
		Log::Error( "DataStoreSQLL", "Failed to open DB: %s", sqlite3_errmsg( m_pDB ) );
		Stop();
		return false;
	}

	const char * DATA_TABLE =
		"CREATE TABLE IF NOT EXISTS data(" \
		"id CHAR(50) PRIMARY KEY NOT NULL," \
		"data TEXT NOT NULL ); " \
		"CREATE TABLE IF NOT EXISTS data_index(" \
		"key TEXT NOT NULL," \
		"value TEXT NOT NULL," \
		"id CHAR(50) NOT NULL," \
		"PRIMARY KEY( key, value)); " \
		"CREATE INDEX IF NOT EXISTS data_index_id ON data_index(id); ";

	if ( sqlite3_exec( m_pDB, DATA_TABLE, NULL, NULL, NULL ) != SQLITE_OK )
	{
		Log::Error( "DataStoreSQLL", "Failed to create tables: %s", sqlite3_errmsg( m_pDB ) );
		Stop();
		return false;
	}

	// start our background thread for storage..
	pTP->InvokeOnThread( VOID_DELEGATE( DataStoreSQLL, ProcessCommandQueue, this ) );
	m_bThreadStopped = false;

	return true;
}
	
bool DataStoreSQLL::Stop()
{
	if ( m_pDB == NULL )
		return false;

	m_bStop = true;
	m_CommandQueued.notify_one();
	while(! m_bThreadStopped )
		boost::this_thread::sleep( boost::posix_time::milliseconds(50) );

	sqlite3_close( m_pDB );
	m_pDB = NULL;

	return true;
}

bool DataStoreSQLL::Drop()
{
	if ( m_pDB == NULL )
		return false;

	m_bStop = true;
	m_CommandQueued.notify_one();
	while(! m_bThreadStopped )
		boost::this_thread::sleep( boost::posix_time::milliseconds(50) );

	sqlite3_close( m_pDB );
	m_pDB = NULL;

	try {
		boost::filesystem::remove( m_DBFile );
	}
	catch( const std::exception & ex )
	{
		Log::Error( "DataStoreSQLL", "Failed to remove DB file: %s", ex.what() );
		return false;
	}

	return true;
}

bool DataStoreSQLL::Save( const std::string & a_ID, const Json::Value & a_Data,
	Delegate<bool> a_Callback /*= Delegate<bool>()*/ )
{
	Json::Value save(a_Data);
	ApplyDefinition(save, m_Definition);
	save["_id"] = a_ID;

	char * pQuery = sqlite3_mprintf("REPLACE INTO data(id,data) VALUES('%q','%q'); ",
		a_ID.c_str(), save.toStyledString().c_str() );
	std::string insert( pQuery );
	sqlite3_free( pQuery );

	Json::Value & indexes = m_Definition["_Indexed"];
	for (size_t i = 0; i < indexes.size(); ++i)
	{
		const std::string & index = indexes[i].asString();
		Json::Value & value = JsonHelpers::Resolve(save, index);

		if (value.isObject() || value.isArray())
		{
			Log::Warning("DataStoreSQLL", "Cannot index object/array.");
			continue;
		}

		char * pQuery = sqlite3_mprintf("REPLACE INTO data_index(key,value,id) VALUES('%q','%q','%q'); ",
			index.c_str(), value.asCString(), a_ID.c_str() );
		insert += pQuery;
		sqlite3_free( pQuery );
	}

	PushCommand( new ExecCommand( insert, a_Callback ) );
	return true;
}

bool DataStoreSQLL::Load( const std::string & a_ID, Delegate<Json::Value *> a_Callback )
{
	std::string query("SELECT data FROM data where id LIKE '" + a_ID + "';");
	PushCommand( new LoadCommand( query, a_Callback, a_ID ) );

	return true;
}

bool DataStoreSQLL::Delete( const std::string & a_ID, Delegate<bool> a_Callback /*= Delegate<bool>()*/ )
{
	std::string query( "DELETE FROM data where id='" + a_ID + "'; DELETE FROM data_index where id='" + a_ID + "';" );
	PushCommand( new ExecCommand( query, a_Callback ) );

	return true;
}

bool DataStoreSQLL::Find( const Conditions & a_Conditions, Delegate<QueryResults *> a_Callback )
{
	std::string query("SELECT DISTINCT id FROM data_index");
	std::string cond(GetWhereClause(a_Conditions));
	if (cond.size() > 0)
		query += " WHERE " + cond;

	PushCommand( new QueryCommand( query, a_Callback ) );
	return true;
}

//-------------------------------------

//! Helper function to merge a definition into some json
void DataStoreSQLL::ApplyDefinition(Json::Value & json, const Json::Value & def)
{
	for (Json::ValueConstIterator iElement = def.begin(); iElement != def.end(); ++iElement)
	{
		const std::string & key = iElement.key().asString();
		if (key[0] == '_')
			continue;		// skip any fields starting with _
		const Json::Value & value = *iElement;

		if (json.isMember(key) && json[key].type() == value.type())
		{
			if (value.type() == Json::arrayValue)
			{
				for (size_t i = 0; i < value.size(); ++i)
					ApplyDefinition(json[i], value[i]);
			}
			else if (value.type() == Json::objectValue)
				ApplyDefinition(json[key], value);
		}
		else
			json[key] = value;
	}
}

std::string DataStoreSQLL::GetWhereOp(Logic::EqualityOp a_Op)
{
	static const char * TEXT[] =
	{
		"=",		// ==
		"!=",		// !=
		">",		// >
		">=",		// >=
		"<",		// <
		"<=",		// <=
		" LIKE "
	};
	int index = (int)a_Op;
	if (index < 0 || index >= sizeof(TEXT) / sizeof(TEXT[0]))
		return "=";
	return TEXT[index];
}

std::string DataStoreSQLL::GetWhereClause(const Conditions & a_Conditions, Logic::LogicalOp a_LogOp /*= Logic::AND*/ )
{
	const std::string WS(" ");
	const std::string BP(" (");
	const std::string EP(") ");
	const std::string SQ("'");

	std::string cond;
	for (Conditions::const_iterator iCond = a_Conditions.begin(); iCond != a_Conditions.end(); ++iCond)
	{
		IConditional * pICond = (*iCond).get();
		if (pICond == NULL)
			continue;

		if (cond.size() > 0)
			cond += WS + Logic::LogicalOpText(a_LogOp) + WS;

		if (pICond->GetRTTI() == EqualityCondition::GetStaticRTTI())
		{
			EqualityCondition * pCond = (EqualityCondition *)pICond;
			if (pCond->m_Value.isObject() || pCond->m_Value.isArray())
				continue;

			std::string value;
			if (pCond->m_Value.isNumeric())
				value = pCond->m_Value.asString();
			else
				value = SQ + pCond->m_Value.asString() + SQ;

			cond += BP
				+ std::string("key='") 
				+ pCond->m_Path 
				+ std::string("' AND value") 
				+ GetWhereOp(pCond->m_EqualOp) 
				+ value 
				+ EP;
		}
		else if (pICond->GetRTTI() == LogicalCondition::GetStaticRTTI())
		{
			LogicalCondition * pLogicalCond = (LogicalCondition *)pICond;
			cond += BP + GetWhereClause(pLogicalCond->m_Conditions, pLogicalCond->m_LogicOp) + EP;
		}
	}

	return cond;
}

void DataStoreSQLL::PushCommand( ICommand * a_pCommand )
{
	m_DBLock.lock();
	m_CommandQueue.push_back( a_pCommand );
	m_CommandQueued.notify_one();
	m_DBLock.unlock();
}

void DataStoreSQLL::ProcessCommandQueue()
{
	boost::unique_lock<boost::mutex> lock( m_DBLock );
	while( true )
	{
		if ( m_CommandQueue.begin() == m_CommandQueue.end() )
		{
			if ( m_bStop )
				break;
			m_CommandQueued.wait(lock);
		}
		// make a copy of the commands so we can process them without keeping a lock
		CommandQueue commands( m_CommandQueue );
		m_CommandQueue.clear();
		// unlock while we are processing the commands so we don't block anyone..
		lock.unlock();

		for( CommandQueue::iterator iCommand = commands.begin();
			iCommand != commands.end(); ++iCommand )
		{
			ICommand * pCommand = *iCommand;
			pCommand->Execute( m_pDB );

			delete pCommand;
		}

		// re-lock before we continue;;
		lock.lock();
	}

	m_bThreadStopped = true;
}


void DataStoreSQLL::ExecCommand::Execute( sqlite3 * a_pDB )
{
	bool bSuccess = true;
	if (sqlite3_exec(a_pDB, m_SQL.c_str(), NULL, NULL, NULL) != SQLITE_OK)
	{
		Log::Error("DataStoreSQLL", "Failed to query data: %s", sqlite3_errmsg(a_pDB));
		bSuccess = false;
	}

	ThreadPool::Instance()->InvokeOnMain<bool>( m_Callback, bSuccess );
}

void DataStoreSQLL::LoadCommand::Execute( sqlite3 * a_pDB )
{
	bool bLoaded = false;

	Json::Value * data = new Json::Value();

	sqlite3_stmt * statement;
	if (sqlite3_prepare_v2(a_pDB, m_SQL.c_str(), m_SQL.size(), &statement, 0) == SQLITE_OK)
	{
		while (sqlite3_step(statement) == SQLITE_ROW)
		{
			const char * pData = (const char *)sqlite3_column_text(statement, 0);

			Json::Reader reader(Json::Features::strictMode());
			if (reader.parse(pData, *data))
			{
				bLoaded = true;
				if (! data->isMember( "_id" ) )
					(*data)["_id"] = m_ID;

				ThreadPool::Instance()->InvokeOnMain<Json::Value *>( m_Callback, data );
				data = new Json::Value();
			}
		}

		sqlite3_finalize(statement);
	}
	else
		Log::Error("DataStoreSQLL", "Failed to query data: %s", sqlite3_errmsg(a_pDB));

	// send last record to indicate the end of loading..
	(*data)["_done"] = true;
	if (! bLoaded )
		(*data)["_error"] = true;
	ThreadPool::Instance()->InvokeOnMain<Json::Value *>( m_Callback, data );
}

void DataStoreSQLL::QueryCommand::Execute( sqlite3 * a_pDB )
{
	QueryResults * results = new QueryResults();

	sqlite3_stmt * statement;
	if (sqlite3_prepare_v2(a_pDB, m_SQL.c_str(), m_SQL.size(), &statement, 0) == SQLITE_OK)
	{
		while (sqlite3_step(statement) == SQLITE_ROW)
			results->push_back(( const char *)sqlite3_column_text(statement, 0));

		sqlite3_finalize(statement);
	}
	else
		Log::Error("DataStoreSQLL", "Failed to query data: %s", sqlite3_errmsg(a_pDB));

	ThreadPool::Instance()->InvokeOnMain<QueryResults *>( m_Callback, results );
}
