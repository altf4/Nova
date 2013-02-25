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

bool Database::Connect()
{
	LOG(DEBUG, "Opening database " + m_databaseFile, "");
	int rc;
	rc = sqlite3_open(m_databaseFile.c_str(), &db);

	if (rc)
	{
		throw DatabaseException(string(sqlite3_errmsg(db)));
	}
	else
	{
		return true;
	}
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
