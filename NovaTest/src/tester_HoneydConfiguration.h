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
#include "HoneydConfiguration/HoneydConfiguration.h"
#include "HoneydConfiguration/Profile.h"
#include "HoneydConfiguration/Node.h"


#define HC HoneydConfiguration::Inst()

using namespace Nova;

// The test fixture for testing class HoneydConfiguration.
class HoneydConfigurationTest : public ::testing::Test
{

protected:
	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:
	void SetUp()
	{
		EXPECT_TRUE(HC != NULL);
		EXPECT_TRUE(HC->ReadAllTemplatesXML());
		HC->ClearProfiles();
		HC->ClearNodes();
	}
};

TEST_F(HoneydConfigurationTest, test_getMaskBits)
{
	EXPECT_EQ(0, HC->GetMaskBits(0));
	EXPECT_EQ(16, HC->GetMaskBits(~0 - 65535));
	EXPECT_EQ(24, HC->GetMaskBits(~0 - 255));
	EXPECT_EQ(31, HC->GetMaskBits(~0 -1));
	EXPECT_EQ(32, HC->GetMaskBits(~0));
	EXPECT_EQ(-1, HC->GetMaskBits(240));
}

TEST_F(HoneydConfigurationTest, test_WriteAllTemplatesXML)
{
	Profile * p = new Profile("default", "TestProfile");
	EXPECT_TRUE(HC->AddProfile(p));
	EXPECT_TRUE(HC->WriteAllTemplatesToXML());
	EXPECT_TRUE(HC->ReadAllTemplatesXML());
	EXPECT_TRUE(HC->GetProfile("TestProfile") != NULL);
}

TEST_F(HoneydConfigurationTest, test_RenameProfile)
{
	// Create dummy profile
	Profile * p = new Profile("default", "TestProfile");

	// Add the dummy profile
	EXPECT_TRUE(HC->AddProfile(p));
	EXPECT_TRUE(HC->GetProfile("TestProfile") != NULL);

	//Test renaming a profile
	EXPECT_TRUE(HC->RenameProfile("TestProfile", "TestProfile-renamed"));

	// Make sure it was renamed
	EXPECT_TRUE(HC->GetProfile("TestProfile-renamed") != NULL);
	EXPECT_TRUE(HC->GetProfile("TestProfile") == NULL);
}

TEST_F(HoneydConfigurationTest, test_errorCases)
{
	EXPECT_FALSE(HC->DeleteProfile(""));
	EXPECT_FALSE(HC->DeleteProfile("aoeustnhaoesnuhaosenuht"));
	EXPECT_FALSE(HC->DeleteNode(""));
	EXPECT_FALSE(HC->DeleteNode("aoeuhaonsehuaonsehu"));
	EXPECT_EQ(NULL, HC->GetProfile(""));
	EXPECT_EQ(NULL, HC->GetProfile("aouhaosnuheaonstuh"));
	EXPECT_EQ(NULL, HC->GetNode(""));
	EXPECT_EQ(NULL, HC->GetNode("aouhaosnuheaonstuh"));

}

TEST_F(HoneydConfigurationTest, test_Profile)
{
	//Create dummy profile
	Profile * p = new Profile("default", "TestProfile");

	//Test adding a profile
	EXPECT_TRUE(HC->AddProfile(p));
	EXPECT_TRUE(HC->GetProfile("TestProfile") != NULL);

	// Add a child profile
	Profile * pChild = new Profile("TestProfile", "TestProfileChild");
	EXPECT_TRUE(HC->AddProfile(pChild));
	EXPECT_TRUE(HC->GetProfile("TestProfileChild") != NULL);

	//Test renaming a profile
	EXPECT_TRUE(HC->RenameProfile("TestProfile", "TestProfileRenamed"));
	EXPECT_TRUE(HC->GetProfile("TestProfile") == NULL);
	EXPECT_TRUE(HC->GetProfile("TestProfileRenamed") != NULL);
	EXPECT_TRUE(HC->GetProfile("TestProfile") == NULL);

	//Test deleting a profile
	EXPECT_TRUE(HC->DeleteProfile("TestProfileRenamed"));
	EXPECT_TRUE(HC->GetProfile("TestProfileRenamed") == NULL);
	EXPECT_TRUE(HC->GetProfile("TestProfileChild") == NULL);
}

TEST_F(HoneydConfigurationTest, test_GetProfileNames)
{
	EXPECT_TRUE(HC->AddProfile(new Profile("default", "top")));
	EXPECT_TRUE(HC->AddProfile(new Profile("default", "top")));
	EXPECT_TRUE(HC->AddProfile(new Profile("top", "topChild")));
	EXPECT_TRUE(HC->AddProfile(new Profile("topChild", "topGrandChild")));

	vector<string> profiles = HC->GetProfileNames();
	// default + 4 new ones (one duplicate) = 4
	EXPECT_EQ(4, profiles.size());
}

TEST_F(HoneydConfigurationTest, test_AddNodes)
{
	EXPECT_TRUE(HC->AddNodes("default", "DHCP", "eth0", 10));
	EXPECT_EQ(10, HC->GetNodeMACs().size());
}

TEST_F(HoneydConfigurationTest, test_AddNode)
{
	Node node;
	node.m_MAC = "FF:FF:BA:BE:CA:FE";
	node.m_pfile = "default";

	EXPECT_TRUE(HC->AddNode(node));
	EXPECT_TRUE(HC->GetNode("FF:FF:BA:BE:CA:FE") != NULL);
	EXPECT_TRUE(HC->GetNode("FF:FF:BA:BE:CA:FE")->m_MAC == "FF:FF:BA:BE:CA:FE");
}
