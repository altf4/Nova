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
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#define HC HoneydConfiguration::Inst()//this is the instantiation of the honeyd singleton config class

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

//create a false profile of random crap and write it
//then read it back in and confirm that the structure is correct
TEST_F(HoneydConfigurationTest, test_WriteAllTemplatesXML)
{
	Profile * p = new Profile("default", "TestProfile");
	Profile * l = new Profile("p","TestProfile2");
	p->SetDropRate("15");
	p->SetPersonality("Linux 3.53");
	p->SetUptimeMax(400000);
	p->SetUptimeMin(0);
	l->SetPersonality("Linux 3.53");
	cout<<p->GetDropRate()<<endl;
	EXPECT_EQ(p->GetDropRate(),"15");
	cout<<p->GetPersonality()<<endl;
	EXPECT_EQ(p->GetPersonality(),"Linux 3.53");
	cout<<p->GetUptimeMax()<<endl;
	EXPECT_EQ(p->GetUptimeMax(),400000);
	cout<<p->GetUptimeMin()<<endl;
	EXPECT_EQ(p->GetUptimeMin(),0);
	cout<<p->GetCount()<<endl;

	//if this of these 4 then its public and we need to set manually
	p->SetDropRate();
	p->SetPersonality();
	p->SetUptimeMax();
	p->SetUptimeMin();
	p->m_avgPortCount = 15;
	p->m_children.size();//contains the number of children that this node has, it should be 0
	/*
	attributes to test:
	std::string GetRandomVendor();
	PortSet *GetRandomPortSet();
	PortSet *GetPortSet(std::string name);
	uint GetVendorCount(std::string vendorName);
	void RecalculateChildDistributions();
	bool Copy(Profile *source);
	std::string GetPersonality();
	std::string GetName();
	uint32_t GetCount();
	std::string GetParentProfile();
	std::vector<std::string> GetVendors();
	std::vector<uint> GetVendorCounts();
	bool IsPersonalityInherited();
	bool IsUptimeInherited();
	double m_distribution;

	*/


	//here the plan is to make profile p a custom profile and we want to
	//verify that the custom profile p actually contains what we give it
	//now the only question is how do we customize p ?!?!?
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
//test read/write permissions and also if the file is installed in the correct location and exists
/*TEST_F(HoneydConfigurationTest,test_WriteProfilesToXML)
{
	//ls -l /etc/passwd
	///home/rami/Code/Nova/Installer/userFiles/config/templates/default/profiles.xml
	struct stat sb;
	char a[100];
	string rename1 = "cd " + home + "; mv profiles.xml profile.xml";
	string rename2 = "cd " + home + "; mv profile.xml profiles.xml";
	string readWriteStatus = "cd " + home + "/profiles.xml";//used to check
	int TempNumOne = readWriteStatus.size();
	for (int l=0;l<=TempNumOne;l++)
		        {
		            a[l]=readWriteStatus[l];
		        }

	 if( stat(a, &sb) == -1 ) {
	        std::cout << "Couldn't stat(). Cannot access file, could assume it doesn't exist\n" << std::endl;
	        //return 1;
	}

	    std::cout << "Permissions: " << std::oct << (unsigned long) sb.st_mode << std::endl;

}*/

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
	EXPECT_TRUE(HC->AddNodes("default", "default", "Dell", "DHCP", "eth0", 10));
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

TEST_F(HoneydConfigurationTest, test_WouldAddProfileCauseNodeDeletions)
{
	Profile *p = new Profile("default", "testProfile");
	p->m_portSets.push_back(new PortSet("test"));
	EXPECT_TRUE(HC->AddProfile(p));

	Node node;
	node.m_pfile == "testProfile";
	node.m_portSetName = "test";
	node.m_MAC = "FF:FF:BA:BE:CA:FE";
	EXPECT_TRUE(HC->AddNode(node));

	Profile *p2 = new Profile("default", "testProfile");
	EXPECT_TRUE(HC->WouldAddProfileCauseNodeDeletions(p2));

	p2->m_portSets.push_back(new PortSet("test"));
	EXPECT_FALSE(HC->WouldAddProfileCauseNodeDeletions(p2));
}

//new test to move file location for bool HoneydConfiguration::ReadScriptsXML()
//	function it should return false and then move it back and run it again and it should return true

TEST_F(HoneydConfigurationTest, test_ReadScriptsXML)
{
	string home = Config::Inst()->GetPathHome() + "/config/templates";
	string command1 = "cd " + home + "; mv scripts.xml script.xml";//change script name from original
	string command2 = "cd " + home + "; mv script.xml scripts.xml";//change script name back to original
	int TempNumOne=command1.size();
	char a[100];
	char b[100];
	for (int l=0;l<=TempNumOne;l++)
	        {
	            a[l]=command1[l];
	        }
	TempNumOne = command2.size();
	for (int l=0;l<=TempNumOne;l++)
		        {
		            b[l]=command2[l];
		        }
	//should return true initially since the xml file should exist in its proper location
	EXPECT_TRUE(HC->ReadScriptsXML());//should pass
	system(a);
	//should confirm that the function returns false because the system command was executed and the file name was changed
	EXPECT_FALSE(HC->ReadScriptsXML());//should pass
	system(b);
	EXPECT_TRUE(HC->ReadScriptsXML());//should pass
}

