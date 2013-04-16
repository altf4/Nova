//============================================================================
// Name        : Database.cpp
// Copyright   : DataSoft Corporation 2011-2013
//	Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Wrapper for adding entries to the SQL database
//============================================================================

#include "Config.h"
#include "Database.h"
#include "NovaUtil.h"
#include "Logger.h"
#include "ClassificationEngine.h"
#include "FeatureSet.h"

#include <iostream>
#include <sstream>
#include <string>

// Quick macro so we don't have to copy/paste this over and over
#define expectReturnValue(val) \
if (res != val ) \
{\
	LOG(ERROR, "SQL error: " + string(sqlite3_errmsg(db)), "");\
	return;\
}

using namespace std;

namespace Nova
{

int Database::callback(void *NotUsed, int argc, char **argv, char **azColName){
	int i;
	for(i=0; i<argc; i++){
		cout << azColName[i] << "=" << (argv[i] ? argv[i] : "NULL") << endl;
	}
	cout << endl;
	return 0;
}


Database::Database(std::string databaseFile)
{
	if (databaseFile == "")
	{
		databaseFile = Config::Inst()->GetPathHome() + "/data/novadDatabase.db";
	}
	m_databaseFile = databaseFile;
}

Database::~Database()
{
	Disconnect();
}

bool Database::Connect()
{
	LOG(DEBUG, "Opening database " + m_databaseFile, "");
	int res;
	res = sqlite3_open(m_databaseFile.c_str(), &db);

	if (res)
	{
		throw DatabaseException(string(sqlite3_errmsg(db)));
	}
	else
	{
		return true;
	}

	res = sqlite3_prepare_v2(db,
	  "INSERT OR REPLACE INTO suspects (ip, "
			"interface, "
			"startTime, "
			"endTime, "
			"lastTime, "
			"classification, "
			"hostileNeighbors, "
			"isHostile, "
			"classificationNotes) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
	  -1, &insertSuspect,  NULL);
	expectReturnValue(SQLITE_OK);
}

bool Database::Disconnect()
{
	sqlite3_finalize(insertSuspect);
}

void Database::ResetPassword()
{
	stringstream ss;
	ss << "REPLACE INTO credentials VALUES (\"nova\", \"934c96e6b77e5b52c121c2a9d9fa7de3fbf9678d\", \"root\")";

	char *zErrMsg = 0;
	int state = sqlite3_exec(db, ss.str().c_str(), callback, 0, &zErrMsg);
	if (state != SQLITE_OK)
	{
		string errorMessage(zErrMsg);
		sqlite3_free(zErrMsg);
		throw DatabaseException(string(errorMessage));
	}
}

void Database::InsertSuspect(Suspect *suspect)
{
	int res;

	res = sqlite3_bind_text(insertSuspect, 1, suspect->GetIpString().c_str(), -1, SQLITE_TRANSIENT);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_text(insertSuspect, 2, suspect->GetInterface().c_str(), -1, SQLITE_TRANSIENT);
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_int64(insertSuspect, 3, static_cast<long int>(suspect->m_features.m_startTime));
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_int64(insertSuspect, 4, static_cast<long int>(suspect->m_features.m_endTime));
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_int64(insertSuspect, 5, static_cast<long int>(suspect->m_features.m_lastTime));
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_double(insertSuspect, 6, suspect->GetClassification());
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_int(insertSuspect, 7, suspect->GetHostileNeighbors());
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_int(insertSuspect, 8, suspect->GetIsHostile());
	expectReturnValue(SQLITE_OK);
	res = sqlite3_bind_text(insertSuspect, 9, suspect->m_classificationNotes.c_str(), -1, SQLITE_TRANSIENT);
	expectReturnValue(SQLITE_OK);


	res = sqlite3_step(insertSuspect);
	expectReturnValue(SQLITE_DONE);
}

void Database::InsertSuspectHostileAlert(Suspect *suspect)
{
	FeatureSet features = suspect->GetFeatureSet(MAIN_FEATURES);

	stringstream ss;
	ss << "INSERT INTO statistics VALUES (NULL";

	for (int i = 0; i < DIM; i++)
	{
		ss << ", ";

		ss << features.m_features[i];

	}
	ss << ");";


	ss << "INSERT INTO suspect_alerts VALUES (NULL, '";
	ss << suspect->GetIpString() << "', '" << suspect->GetInterface() << "', datetime('now')" << ",";
	ss << "last_insert_rowid()" << "," << suspect->GetClassification() << ")";

	char *zErrMsg = 0;
	int state = sqlite3_exec(db, ss.str().c_str(), callback, 0, &zErrMsg);
	if (state != SQLITE_OK)
	{
		string errorMessage(zErrMsg);
		sqlite3_free(zErrMsg);
		throw DatabaseException(string(errorMessage));
	}
}

}/* namespace Nova */
