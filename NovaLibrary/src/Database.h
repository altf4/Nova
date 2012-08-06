//============================================================================
// Name        : Database.h
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

#ifndef DATABASE_H_
#define DATABASE_H_

#include "Suspect.h"
#include "FeatureSet.h"

#include <string>
#include <mysql/mysql.h>


namespace Nova
{

class DatabaseException : public std::runtime_error
{
public:
	DatabaseException(const string& errorcode)
	: runtime_error("Database error code: " + errorcode) {};

};

class Database
{
public:
	Database(std::string host = "127.0.0.1", std::string username = "root", std::string password = "root", std::string database = "nova");
	bool Connect();

	int InsertSuspectHostileAlert(Suspect *suspect);
	int InsertStatisticsPoint(FeatureSet *features);


private:
	std::string m_host;
	std::string m_username;
	std::string m_password;
	std::string m_database;

	MYSQL *connection, db;
};

} /* namespace Nova */
#endif /* DATABASE_H_ */
