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
#include "FeatureSet.h"

using namespace Nova;
using namespace std;

namespace {
// The test fixture for testing class Suspect.
class SuspectTest : public ::testing::Test {
protected:
	// Objects declared here can be used by all tests in the test case
	Suspect *suspect;
	Packet p1, p2;


	SuspectTest() {
		suspect = new Suspect();

		// First test packet
		p1.ip_hdr.ip_p = 6;
		p1.fromHaystack = false;
		// These are just made up input values that make the math easy
		p1.tcp_hdr.dest = 80;
		p1.ip_hdr.ip_dst.s_addr = 1;

		// Note: the byte order gets flipped for this
		p1.ip_hdr.ip_len = (u_short)1;
		p1.pcap_header.ts.tv_sec = 10;


		// Second test packet
		p2.ip_hdr.ip_p = 6;
		p2.fromHaystack = true;
		p2.tcp_hdr.dest = 20;
		p2.ip_hdr.ip_dst.s_addr = 2;

		// Note: the byte order gets flipped for this
		p2.ip_hdr.ip_len = (u_short)1;
		p2.pcap_header.ts.tv_sec = 20;
	}

};

// Check adding and removing evidence
TEST_F(SuspectTest, EvidenceAddingRemoving)
{
	EXPECT_EQ(0, suspect->AddEvidence(p1));
	EXPECT_EQ(0, suspect->AddEvidence(p2));
	EXPECT_TRUE(suspect->GetNeedsClassificationUpdate());

	EXPECT_EQ(0, memcmp(&suspect->GetEvidence().at(0), &p1, sizeof(p1)));
	EXPECT_EQ(0, memcmp(&suspect->GetEvidence().at(1), &p2, sizeof(p2)));

	EXPECT_EQ(0, suspect->ClearEvidence());
	EXPECT_EQ((uint)0, suspect->GetEvidence().size());
}

TEST_F(SuspectTest, EvidenceProcessing)
{
	EXPECT_EQ(0, suspect->AddEvidence(p1));
	EXPECT_EQ(0, suspect->AddEvidence(p2));

	// This isn't exactly an intuitive set of calls to update the evidence...
	// Convert the packets to evidence
	EXPECT_EQ(0, suspect->UpdateEvidence());
	// Calculate the feature values from the evidence
	EXPECT_EQ(0, suspect->CalculateFeatures());
	// Move this stuff from the unsent evidence to the normal evidence
	EXPECT_NO_FATAL_FAILURE(suspect->UpdateFeatureData(true));

	// Do we have the correct packet count?
	EXPECT_EQ((uint)2, suspect->GetFeatureSet().m_packetCount);


	FeatureSet fset = suspect->GetFeatureSet();

	// Note: This is stolen from our featureSet test code
	EXPECT_EQ(fset.m_features[IP_TRAFFIC_DISTRIBUTION], 1);
	EXPECT_EQ(fset.m_features[PORT_TRAFFIC_DISTRIBUTION], 1);
	EXPECT_EQ(fset.m_features[HAYSTACK_EVENT_FREQUENCY], 0.1);
	EXPECT_EQ(fset.m_features[PACKET_SIZE_DEVIATION], 0);
	EXPECT_EQ(fset.m_features[PACKET_SIZE_MEAN], 256);
	EXPECT_EQ(fset.m_features[DISTINCT_IPS], 2);
	EXPECT_EQ(fset.m_features[DISTINCT_PORTS], 2);
	EXPECT_EQ(fset.m_features[PACKET_INTERVAL_MEAN], 10);
	EXPECT_EQ(fset.m_features[PACKET_INTERVAL_DEVIATION], 0);
}

// Check that someMethod functions
TEST_F(SuspectTest, test_OwnerFunctionality)
{
	EXPECT_FALSE(suspect->HasOwner());

	EXPECT_NO_FATAL_FAILURE(suspect->SetOwner());
	EXPECT_TRUE(suspect->HasOwner());
	EXPECT_TRUE(pthread_equal(suspect->GetOwner(), pthread_self()));

	EXPECT_EQ(0, suspect->ResetOwner());
	EXPECT_FALSE(suspect->HasOwner());
}

TEST_F(SuspectTest, Serialization)
{
	// Just setup to get a suspect to serialize
	suspect->AddEvidence(p1);
	suspect->AddEvidence(p2);
	EXPECT_EQ(0, suspect->UpdateEvidence());
	EXPECT_EQ(0, suspect->CalculateFeatures());
	EXPECT_NO_FATAL_FAILURE(suspect->UpdateFeatureData(true));

	u_char buffer[MAX_MSG_SIZE];
	suspect->Serialize(&buffer[0], true);

	Suspect *suspectCopy = new Suspect();
	suspectCopy->Deserialize(&buffer[0], true, false);

	EXPECT_EQ(*suspect, *suspectCopy);

	EXPECT_NO_FATAL_FAILURE(delete suspectCopy);
}

TEST_F(SuspectTest, test_locking)
{
	// WARNING: These may deadlock if there are problems
	suspect->RdlockSuspect();
	suspect->UnlockSuspect();

	suspect->WrlockSuspect();
	suspect->UnlockSuspect();

	suspect->SetOwner();
	suspect->WrlockSuspect();
	suspect->UnlockSuspect();

	suspect->RdlockSuspect();
	suspect->UnlockSuspect();
	suspect->ResetOwner();
}

}

