//============================================================================
// Name        : tester_Database.h
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
// Description : This file contains unit tests for the class Database
//============================================================================/*

#include "gtest/gtest.h"

#include "Database.h"

using namespace Nova;

// The test fixture for testing class Database.
class DatabaseTest : public ::testing::Test {
protected:
	// Objects declared here can be used by all tests in the test case
	Database testObject;

	// Unused methods here may be deleted
	DatabaseTest() {
		// You can do set-up work for each test here.
		testObject.Connect();
	}
	virtual ~DatabaseTest() {
		// You can do clean-up work that doesn't throw exceptions here.
	}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:
	virtual void SetUp() {
		// Code here will be called immediately after the constructor (right before each test).
	}

	virtual void TearDown() {
		// Code here will be called immediately after each test (right before the destructor).
	}
};

// Tests go here. Multiple small tests are better than one large test, as each test
// will get a pass/fail and debugging information associated with it.

TEST_F(DatabaseTest, testInsertHostileSuspectEvent)
{
	FeatureSet f;
	for (int i = 0; i < DIM; i++)
	{
		f.m_features[i] = i;
	}
	Suspect s;
	s.SetFeatureSet(&f, MAIN_FEATURES);
	s.SetClassification(0.42);

	testObject.InsertSuspectHostileAlert(&s);
}
