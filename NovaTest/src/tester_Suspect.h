//============================================================================
// Name        : tester_Suspect.h
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
// Description : This file contains unit tests for the class Suspect
//============================================================================/*

#include "gtest/gtest.h"

#include "Suspect.h"

using namespace Nova;
using namespace std;

namespace {
// The test fixture for testing class Suspect.
class SuspectTest : public ::testing::Test {
protected:
	// Objects declared here can be used by all tests in the test case
	Suspect *testObject;

	// Unused methods here may be deleted
	SuspectTest() {
		testObject = new Suspect();
	}
};


// Check that someMethod functions
TEST_F(SuspectTest, test_OwnerFunctionality)
{
	EXPECT_FALSE(testObject->HasOwner());

	EXPECT_NO_FATAL_FAILURE(testObject->SetOwner());
	EXPECT_TRUE(testObject->HasOwner());
	EXPECT_TRUE(pthread_equal(testObject->GetOwner(), pthread_self()));

	EXPECT_EQ(0, testObject->ResetOwner());
	EXPECT_FALSE(testObject->HasOwner());

}
}

