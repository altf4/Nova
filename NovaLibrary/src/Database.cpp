//============================================================================
// Name        : Database.cpp
// Copyright   : DataSoft Corporation 2011-2012
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
// Description : 
//============================================================================/*

#include "Config.h"
#include "Database.h"
#include "NovaUtil.h"

#include <iostream>
#include <sstream>


using namespace std;

namespace Nova
{

Database::Database(string host, string username, string password, string database):
		m_host(host)
		, m_username(username)
		, m_password(password)
		, m_database(database)
{
	mysql_init(&db);
	my_bool reconnect = true;
	mysql_options(&db, MYSQL_OPT_RECONNECT, &reconnect);
}

bool Database::Connect()
{
	connection = mysql_real_connect(&db, m_host.c_str(), m_username.c_str(), m_password.c_str(), m_database.c_str(), 0,0,0);

	if (connection == NULL)
	{
		throw DatabaseException(ConvertInt(mysql_errno(&db)) + string(mysql_error(&db)));
	}
	else
	{
		return true;
	}
}

int Database::InsertSuspectHostileAlert(Suspect *suspect)
{
	FeatureSet f = suspect->GetFeatureSet(MAIN_FEATURES);
	int featureSetId = InsertStatisticsPoint(&f);

	stringstream ss;
	ss << "INSERT INTO suspect_alerts VALUES (NULL, '";
	ss << suspect->GetIpString() << "', " << "CURRENT_TIMESTAMP" << ",";
	ss << featureSetId << "," << suspect->GetClassification() << ")";

	int state = mysql_query(connection, ss.str().c_str());
	if (state)
	{
		throw DatabaseException(ConvertInt(mysql_errno(&db)) + string(mysql_error(&db)));
	}
	else
	{
		return mysql_insert_id(&db);
	}
}


int Database::InsertStatisticsPoint(FeatureSet *features)
{
	int state;
	stringstream ss;
	ss << "INSERT INTO statistics VALUES (NULL";

	for (int i = 0; i < DIM; i++)
	{
		ss << ", ";

		if (Config::Inst()->IsFeatureEnabled(i))
		{
			ss << features->m_features[i];
		}
		else
		{
			ss << "NULL";
		}
	}

	ss << ")";

	state = mysql_query(connection, ss.str().c_str());
	if (state)
	{
		throw DatabaseException(ConvertInt(mysql_errno(&db)) + string(mysql_error(&db)));
	}
	else
	{
		return mysql_insert_id(&db);
	}
}


}/* namespace Nova */
