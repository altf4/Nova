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
//============================================================================

#include "gtest/gtest.h"
#include "HoneydConfiguration.h"

using namespace Nova;

// The test fixture for testing class HoneydConfiguration.
class HoneydConfigurationTest : public ::testing::Test
{

protected:

	// Objects declared here can be used by all tests in the test case
	HoneydConfiguration * m_config;

	HoneydConfigurationTest()
	{
		m_config = new HoneydConfiguration();
	}

	~HoneydConfigurationTest()
	{
		delete m_config;
	}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:
	void SetUp()
	{
		EXPECT_TRUE(m_config != NULL);
		EXPECT_TRUE(m_config->LoadAllTemplates());
	}

	void TearDown()
	{

	}
};

// Tests go here. Multiple small tests are better than one large test, as each test
// will get a pass/fail and debugging information associated with it.

TEST_F(HoneydConfigurationTest, test_getMaskBits)
{
	EXPECT_EQ(0, m_config->GetMaskBits(0));
	EXPECT_EQ(16, m_config->GetMaskBits(~0 - 65535));
	EXPECT_EQ(24, m_config->GetMaskBits(~0 - 255));
	EXPECT_EQ(31, m_config->GetMaskBits(~0 -1));
	EXPECT_EQ(32, m_config->GetMaskBits(~0));
}

TEST_F(HoneydConfigurationTest, test_AddPort)
{
	stringstream ss;
	ss.str("");
	EXPECT_TRUE(!(ss.str().compare(m_config->AddPort(0, TCP, OPEN, ""))));
	ss.str("1_TCP_open");
	EXPECT_TRUE(!(ss.str().compare(m_config->AddPort(1, TCP, OPEN, ""))));
	ss.str("65535_UDP_block");
	EXPECT_TRUE(!(ss.str().compare(m_config->AddPort(~0, UDP, RESET, ""))));
	ss.str("65535_TCP_reset");
	EXPECT_TRUE(!(ss.str().compare(m_config->AddPort(~0, TCP, RESET, ""))));
	ss.str("65534_TCP_block");
	EXPECT_TRUE(!(ss.str().compare(m_config->AddPort(431, TCP, BLOCK, ""))));

	std::vector<std::string> scriptNames;
	EXPECT_NO_FATAL_FAILURE(scriptNames = m_config->GetScriptNames());
	uint i = 2;
	while(!scriptNames.empty())
	{
		ss.str("");
		ss << i << "_TCP_" << scriptNames.back();
		EXPECT_TRUE(!(ss.str().compare(m_config->AddPort(i, TCP, SCRIPT, scriptNames.back()))));
		scriptNames.pop_back();
		i++;
	}
}

