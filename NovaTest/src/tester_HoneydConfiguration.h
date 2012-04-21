//============================================================================
// Name        : tester_HoneydConfiguration.h
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
// Description : This file contains unit tests for the class HoneydConfiguration
//============================================================================/*

#include "gtest/gtest.h"

#include "HoneydConfiguration.h"

using namespace Nova;
using namespace std;

// The test fixture for testing class HoneydConfiguration.
class HoneydConfigurationTest : public ::testing::Test {
protected:
	HoneydConfiguration *testObject;

	// Unused methods here may be deleted
	HoneydConfigurationTest() {
		testObject = new HoneydConfiguration();
	}

	virtual void SetUp() {
		testObject->LoadAllTemplates();
	}


};

// Check that someMethod functions
TEST_F(HoneydConfigurationTest, test_AddNewNodes)
{
	EXPECT_TRUE(testObject->AddNewNodes("LinuxServer", "DHCP", "eth0", "", 10));

	// This fails on the test serve with permission errors,
	//testObject->SaveAllTemplates();
}

// TODO: Write a lot more tests

