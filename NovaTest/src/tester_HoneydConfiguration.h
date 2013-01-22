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

TEST_F(HoneydConfigurationTest, test_writeConfig)
{
	EXPECT_TRUE(HC->WriteHoneydConfiguration(Config::Inst()->GetPathHome()));
}
TEST_F(HoneydConfigurationTest, test_configFunctions)
{
	Node node;
	node.m_enabled = true;
	Profile *defaultProfile = new Profile("default", "DefaultProfile");
	Profile *child1 = new Profile(defaultProfile, "child1");
	Profile *child2 = new Profile(defaultProfile, "child2");
	Profile *child3 = new Profile(child1, "child3, child of child1");
	//Profile child5 = Profile();
	defaultProfile->SetDropRate("15");
	defaultProfile->SetPersonality("Linux 3.53");
	defaultProfile->SetUptimeMax(400000);
	defaultProfile->SetUptimeMin(0);
	child1->SetDropRate("20");
	child1->SetPersonality("Windows 8");
	child1->SetUptimeMax(100000);
	child1->SetUptimeMin(0);
	child2->SetDropRate("15");
	child2->SetPersonality("Ubuntu 12");
	child2->SetUptimeMax(200000);
	child2->SetUptimeMin(0);
	child3->SetDropRate("13");
	child3->SetPersonality("Fedora");
	child3->SetUptimeMax(300000);
	child3->SetUptimeMin(0);
	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	EXPECT_TRUE(HC->AddProfile(child1));
	EXPECT_TRUE(HC->AddProfile(child2));
	EXPECT_TRUE(HC->AddProfile(child3));
	EXPECT_TRUE(HC->WriteAllTemplatesToXML());
	EXPECT_TRUE(HC->ReadAllTemplatesXML());
	node.m_MAC= "FF:FF:BA:BE:CA:FE";
	node.m_pfile = "Fedora";
	EXPECT_TRUE(HC->ReadNodesXML());
	EXPECT_TRUE(HC->AddNode(node));
	EXPECT_TRUE(HC->WriteHoneydConfiguration());
	EXPECT_TRUE(HC->WriteNodesToXML());
	EXPECT_TRUE(HC->ReadNodesXML());
}

TEST_F(HoneydConfigurationTest, test_writeAllTemplatesXML)
{
	Profile * defaultProfile = new Profile("default", "DefaultProfile");
	Profile * child1 = new Profile(defaultProfile, "child1");
	Profile * child2 = new Profile(defaultProfile, "child2");
	Profile * child3 = new Profile(child1, "child3, child of child1");
	defaultProfile->SetDropRate("15");
	defaultProfile->SetPersonality("Linux 3.53");
	defaultProfile->SetUptimeMax(400000);
	defaultProfile->SetUptimeMin(0);
	child1->SetDropRate("20");
	child1->SetPersonality("Windows 8");
	child1->SetUptimeMax(100000);
	child1->SetUptimeMin(0);
	child2->SetDropRate("15");
	child2->SetPersonality("Ubuntu 12");
	child2->SetUptimeMax(200000);
	child2->SetUptimeMin(0);
	child3->SetDropRate("13");
	child3->SetPersonality("Fedora");
	child3->SetUptimeMax(300000);
	child3->SetUptimeMin(0);
	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	EXPECT_TRUE(HC->AddProfile(child1));
	EXPECT_TRUE(HC->AddProfile(child2));
	EXPECT_TRUE(HC->AddProfile(child3));
	EXPECT_TRUE(HC->WriteAllTemplatesToXML());
	EXPECT_TRUE(HC->ReadAllTemplatesXML());
	EXPECT_TRUE(HC->GetProfile("DefaultProfile")!=NULL);
	EXPECT_TRUE(HC->GetProfile("DefaultProfile")->GetDropRate().compare("15") == 0);
	EXPECT_TRUE(HC->GetProfile("DefaultProfile")->GetPersonality().compare("Linux 3.53") == 0);
	EXPECT_EQ(400000,HC->GetProfile("DefaultProfile")->GetUptimeMax());
	EXPECT_EQ(0,HC->GetProfile("DefaultProfile")->GetUptimeMin());
	EXPECT_TRUE(HC->GetProfile("child1")!=NULL);
	EXPECT_TRUE(HC->GetProfile("child1")->GetDropRate().compare("20") == 0);
	EXPECT_TRUE(HC->GetProfile("child1")->GetPersonality().compare("Windows 8") == 0);
	EXPECT_EQ(100000,HC->GetProfile("child1")->GetUptimeMax());
	EXPECT_EQ(0,HC->GetProfile("child1")->GetUptimeMin());
	EXPECT_TRUE(HC->GetProfile("child2")!=NULL);
	EXPECT_TRUE(HC->GetProfile("child2")->GetDropRate().compare("15") == 0);
	EXPECT_TRUE(HC->GetProfile("child2")->GetPersonality().compare("Ubuntu 12") == 0);
	EXPECT_EQ(200000,HC->GetProfile("child2")->GetUptimeMax());
	EXPECT_EQ(0,HC->GetProfile("child2")->GetUptimeMin());
	EXPECT_TRUE(HC->GetProfile("child3, child of child1")!=NULL);
	EXPECT_TRUE(HC->GetProfile("child3, child of child1")->GetDropRate().compare("13") == 0);
	EXPECT_TRUE(HC->GetProfile("child3, child of child1")->GetPersonality().compare("Fedora") == 0);
	EXPECT_EQ(300000,HC->GetProfile("child3, child of child1")->GetUptimeMax());
	EXPECT_EQ(0,HC->GetProfile("child3, child of child1")->GetUptimeMin());
	EXPECT_TRUE(HC->GetProfile("DefaultProfile")->m_children[0]->m_name.compare("child1")==0);
	EXPECT_TRUE(HC->GetProfile("DefaultProfile")->m_children[1]->m_name.compare("child2")==0);
	EXPECT_TRUE(HC->GetProfile("child1")->m_children[0]->m_name.compare("child3, child of child1")==0);
}
TEST_F(HoneydConfigurationTest, test_isEquals)
{
	Profile *defaultProfile = new Profile("default","DefaultProfile");
	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	EXPECT_TRUE(defaultProfile->IsEqual(*defaultProfile));
	EXPECT_TRUE(HC->WriteAllTemplatesToXML());
	EXPECT_TRUE(HC->ReadAllTemplatesXML());
}

TEST_F(HoneydConfigurationTest, test_isEqualsRecursive)
{
	Profile *defaultProfile = new Profile("default","DefaultProfile");
	Profile *child1 = new Profile(defaultProfile,"child1");
	Profile *child2 = new Profile(defaultProfile,"child2");
	Profile *child3 = new Profile(defaultProfile,"child3");
	Profile *child4 = new Profile(child3,"child4");
	Profile *child5 = new Profile(defaultProfile,"child5");
	Profile *child6 = new Profile(defaultProfile,"child6");
	Profile *child7 = new Profile(child1,"child7, child of 1");
	defaultProfile->m_osclass = "linux";
	defaultProfile->SetDropRate("15");
	defaultProfile->m_count = 32;
	defaultProfile->SetPersonality("Lethal");
	defaultProfile->SetUptimeMax(15);
	defaultProfile->SetUptimeMin(0);
	child1->m_osclass = "linux";
	child1->SetDropRate("42");
	child1->m_count = 1;
	child1->SetPersonality("Lethal");
	child1->SetUptimeMax(80000);
	child1->SetUptimeMin(1400);
	child2->m_osclass = "linux";
	child2->SetDropRate("200");
	child2->m_count = 50;
	child2->SetPersonality("Thine enemy");
	child2->SetUptimeMax(15);
	child2->SetUptimeMin(0);
	child3->m_osclass = "FREE BSD";
	child3->SetDropRate("515");
	child3->m_count = 12;
	child3->SetPersonality("thugs");
	child3->SetUptimeMax(15);
	child3->SetUptimeMin(0);
	child4->m_osclass = "Windows";
	child4->SetDropRate("800");
	child4->m_count = 25;
	child4->SetPersonality("threat");
	child4->SetUptimeMax(500);
	child4->SetUptimeMin(0);
	child5->m_osclass = "linux";
	child5->SetDropRate("500");
	child5->m_count = 14;
	child5->SetPersonality("moderate");
	child5->SetUptimeMax(300);
	child5->SetUptimeMin(10);
	child6->m_osclass = "linux";
	child6->SetDropRate("215");
	child6->m_count = 20;
	child6->SetPersonality("Lethal");
	child6->SetUptimeMax(400);
	child6->SetUptimeMin(0);


	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	EXPECT_TRUE(HC->AddProfile(child1));
	EXPECT_TRUE(HC->AddProfile(child2));
	EXPECT_TRUE(HC->AddProfile(child3));
	EXPECT_TRUE(HC->AddProfile(child4));
	EXPECT_TRUE(HC->AddProfile(child5));
	EXPECT_TRUE(HC->AddProfile(child6));
	EXPECT_TRUE(HC->AddProfile(child7));
	EXPECT_TRUE(HC->WriteAllTemplatesToXML());
	EXPECT_TRUE(HC->ReadAllTemplatesXML());
	EXPECT_TRUE(defaultProfile->IsEqualRecursive(*defaultProfile));

}

TEST_F(HoneydConfigurationTest, test_toString)
{
	Profile *defaultProfile = new Profile("default","DefaultProfile");
	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	//EXPECT_TRUE(HC->WriteAllTemplatesToXML());
	EXPECT_TRUE(HC->ReadAllTemplatesXML());
	EXPECT_TRUE(defaultProfile->ToString().compare(""));
}

TEST_F(HoneydConfigurationTest, test_getRandomVendor)
{
	Node node1,node2,node3,node4;
	node1.m_IP = "184.185.173.186";
	node2.m_IP = "184.185.173.185";
	node3.m_IP = "184.185.173.184";
	node4.m_IP = "184.185.173.183";
	HC->AddNode(node1);
	HC->AddNode(node2);
	HC->AddNode(node3);
	HC->AddNode(node4);
	Profile *defaultProfile = new Profile("default","DefaultProfile");
	EXPECT_TRUE(HC->AddProfile(defaultProfile));
	EXPECT_TRUE(HC->WriteAllTemplatesToXML());
	EXPECT_TRUE(HC->ReadAllTemplatesXML());
	HC->WriteHoneydConfiguration();
	cout<<"random vendor: "<<defaultProfile->GetRandomVendor()<<endl;
	//EXPECT_TRUE(defaultProfile->GetRandomVendor());
}

TEST_F(HoneydConfigurationTest, test_profileWriteDelete)
{
		Profile * profileDefault = new Profile("default", "TestProfile");
		Profile * profileChild1 = new Profile(profileDefault,"Child1");
		Profile * profileChild2 = new Profile(profileDefault,"Child2");
		Profile * profileChild3 = new Profile(profileDefault,"Child3");
		Profile * profileChild4 = new Profile(profileDefault,"Child4");
		Profile * profileChild5 = new Profile(profileChild4,"Child5");
		EXPECT_TRUE(HC->AddProfile(profileDefault));
		EXPECT_TRUE(HC->AddProfile(profileChild1));
		EXPECT_TRUE(HC->AddProfile(profileChild2));
		EXPECT_TRUE(HC->AddProfile(profileChild3));
		EXPECT_TRUE(HC->AddProfile(profileChild4));
		EXPECT_TRUE(HC->AddProfile(profileChild5));
		EXPECT_TRUE(HC->WriteAllTemplatesToXML());
		EXPECT_TRUE(HC->ReadAllTemplatesXML());
		EXPECT_TRUE(HC->GetProfile("Child1")!=NULL);
		EXPECT_TRUE(HC->DeleteProfile("Child1"));
		EXPECT_TRUE(HC->WriteAllTemplatesToXML());
		EXPECT_TRUE(HC->ReadAllTemplatesXML());
		EXPECT_TRUE(HC->GetProfile("Child1")==NULL);
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

TEST_F(HoneydConfigurationTest, test_readScriptsXML)
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

