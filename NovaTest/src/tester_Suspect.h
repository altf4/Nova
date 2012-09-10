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
	Evidence p1, p2;


	SuspectTest() {
		suspect = new Suspect();

		// First test packet
		p1.m_evidencePacket.ip_p = 6;
		// These are just made up input values that make the math easy
		p1.m_evidencePacket.dst_port = 80;
		p1.m_evidencePacket.ip_dst = 1;
		p1.m_evidencePacket.ip_src = 123456;

		// Note: the byte order gets flipped for this
		p1.m_evidencePacket.ip_len = (uint16_t)256;
		p1.m_evidencePacket.ts = 10;


		// Second test packet
		p2.m_evidencePacket.ip_p = 6;
		p2.m_evidencePacket.dst_port = 20;
		p2.m_evidencePacket.ip_dst = 2;
		p2.m_evidencePacket.ip_src = 98765;

		// Note: the byte order gets flipped for this
		p2.m_evidencePacket.ip_len = (uint16_t)256;
		p2.m_evidencePacket.ts = 20;


		Config::Inst()->EnableAllFeatures();
	}

};

// Check adding and removing evidence
TEST_F(SuspectTest, EvidenceAddingRemoving)
{
	Evidence *t1 = new Evidence();
	Evidence *t2 = new Evidence();
	*t1 = p1;
	*t2 = p2;
	EXPECT_NO_FATAL_FAILURE(suspect->ConsumeEvidence(t1));
	EXPECT_NO_FATAL_FAILURE(suspect->ConsumeEvidence(t2));
}

TEST_F(SuspectTest, EvidenceProcessing)
{
	Evidence *t1 = new Evidence();
	Evidence *t2 = new Evidence();
	*t1 = p1;
	*t2 = p2;
	EXPECT_NO_FATAL_FAILURE(suspect->ConsumeEvidence(t1));
	EXPECT_NO_FATAL_FAILURE(suspect->ConsumeEvidence(t2));

	// Calculate the feature values from the evidence
	EXPECT_NO_FATAL_FAILURE(suspect->CalculateFeatures());
	// Move this stuff from the unsent evidence to the normal evidence
	//EXPECT_NO_FATAL_FAILURE(suspect->UpdateFeatureData(true));

	// Do we have the correct packet count?
	EXPECT_EQ((uint)2, suspect->GetFeatureSet(MAIN_FEATURES).m_packetCount);


	FeatureSet fset = suspect->GetFeatureSet(MAIN_FEATURES);

	// Note: This is stolen from our featureSet test code
	EXPECT_EQ(fset.m_features[IP_TRAFFIC_DISTRIBUTION], 1);
	EXPECT_EQ(fset.m_features[PORT_TRAFFIC_DISTRIBUTION], 1);
	EXPECT_EQ(fset.m_features[PACKET_SIZE_DEVIATION], 0);
	EXPECT_EQ(fset.m_features[PACKET_SIZE_MEAN], 256);
	EXPECT_EQ(fset.m_features[DISTINCT_IPS], 2);
	EXPECT_EQ(fset.m_features[DISTINCT_PORTS], 2);
	EXPECT_EQ(fset.m_features[PACKETS_PER_SECOND], 0);
	EXPECT_EQ(fset.m_features[PACKET_INTERVAL_DEVIATION], 0);
}


TEST_F(SuspectTest, Serialization)
{
	Evidence *t1 = new Evidence();
	Evidence *t2 = new Evidence();
	*t1 = p1;
	*t2 = p2;
	// Just setup to get a suspect to serialize
	EXPECT_NO_FATAL_FAILURE(suspect->ConsumeEvidence(t1));
	EXPECT_NO_FATAL_FAILURE(suspect->ConsumeEvidence(t2));
	EXPECT_NO_FATAL_FAILURE(suspect->CalculateFeatures());
	//EXPECT_NO_FATAL_FAILURE(suspect->UpdateFeatureData(true));

	u_char buffer[MAX_MSG_SIZE];
	EXPECT_NO_FATAL_FAILURE(suspect->Serialize(&buffer[0], MAX_MSG_SIZE, MAIN_FEATURE_DATA));

	Suspect *suspectCopy = new Suspect();
	EXPECT_NO_FATAL_FAILURE(suspectCopy->Deserialize(&buffer[0], MAX_MSG_SIZE, MAIN_FEATURE_DATA));

	EXPECT_EQ(*suspect, *suspectCopy);

	EXPECT_NO_FATAL_FAILURE(delete suspectCopy);
}

}

