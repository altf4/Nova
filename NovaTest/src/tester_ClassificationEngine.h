//============================================================================
// Name        : tester_ClassificationEngine.h
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
// Description : This file contains unit tests for the class ClassificationEngine
//============================================================================/*

#include "gtest/gtest.h"

#include "ClassificationEngine.h"

using namespace Nova;

// The test fixture for testing class ClassificationEngine.
class ClassificationEngineTest : public ::testing::Test
{

protected:

	SuspectTable suspects;
	ClassificationEngine *testObject;

	// Unused methods here may be deleted
	ClassificationEngineTest()
	{
		testObject = new ClassificationEngine(suspects);
	}
};

// Check that someMethod functions
TEST_F(ClassificationEngineTest, test_someMethod)
{
	bool isDmEn = Config::Inst()->GetIsDmEnabled();
	Config::Inst()->SetIsDmEnabled(false);
	EXPECT_EQ(0.42, ClassificationEngine::Normalize(LINEAR, 42, 0, 100));
	Config::Inst()->SetIsDmEnabled(isDmEn);
}
