//============================================================================
// Name        : tester_MessageSerilaization.h
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
// Description : This file contains unit tests for the class MessageSerilaization
//============================================================================/*

#include "gtest/gtest.h"

#include "messaging/messages/RequestMessage.h"
#include "messaging/messages/ControlMessage.h"
#include "messaging/messages/UpdateMessage.h"

using namespace Nova;

// The test fixture for testing class MessageSerilaization.
class MessageSerilaizationTest : public ::testing::Test {
protected:
	uint32_t bufferSize;
	char *buffer;

	MessageSerilaizationTest() {}
};


TEST_F(MessageSerilaizationTest, REQUEST_SUSPECT)
{
	// Create and serialize a message
	RequestMessage msg(REQUEST_SUSPECT);
	SuspectID_pb *temp = msg.m_contents.add_m_suspectid();
	temp->set_m_ip(42);
	temp->set_m_ifname("fake0");
	buffer = msg.Serialize(&bufferSize);

	// Deserialize it and make sure relevant internal variables for that message got copied okay
	// The 4 offset is hacky, but it's to strip off the message size that would normally be removed in Message::Deserialize
	RequestMessage deserializedCopy(buffer + 4, bufferSize);
	EXPECT_EQ(msg.m_contents.m_suspectid(0).SerializeAsString(), deserializedCopy.m_contents.m_suspectid(0).SerializeAsString());
}

TEST_F(MessageSerilaizationTest, REQEST_SUSPECT_REPLY)
{
	RequestMessage msg(REQUEST_SUSPECT_REPLY);
}

TEST_F(MessageSerilaizationTest, REQUEST_SUSPECT_LIST)
{
	RequestMessage msg(REQUEST_SUSPECTLIST_REPLY);

	SuspectID_pb *temp1 = msg.m_contents.add_m_suspectid();
	temp1->set_m_ip(1);
	temp1->set_m_ifname("fake1");

	SuspectID_pb *temp2 = msg.m_contents.add_m_suspectid();
	temp2->set_m_ip(2);
	temp2->set_m_ifname("fakeTestaoeuaoeuaoeu2");

	SuspectID_pb *temp3 = msg.m_contents.add_m_suspectid();
	temp3->set_m_ip(3);
	temp3->set_m_ifname("f3");

	buffer = msg.Serialize(&bufferSize);

	RequestMessage deserializedCopy(buffer + 4, bufferSize);
	EXPECT_EQ(msg.m_contents.m_suspectid(0).SerializeAsString(), deserializedCopy.m_contents.m_suspectid(0).SerializeAsString());
	EXPECT_EQ(msg.m_contents.m_suspectid(1).SerializeAsString(), deserializedCopy.m_contents.m_suspectid(1).SerializeAsString());
	EXPECT_EQ(msg.m_contents.m_suspectid(2).SerializeAsString(), deserializedCopy.m_contents.m_suspectid(2).SerializeAsString());
}

TEST_F(MessageSerilaizationTest, REQUEST_SUSPECT_LIST_empty)
{
	RequestMessage msg(REQUEST_SUSPECTLIST_REPLY);
	buffer = msg.Serialize(&bufferSize);

	RequestMessage deserializedCopy(buffer + 4, bufferSize);
	EXPECT_EQ(msg.m_contents.m_suspectid_size(), deserializedCopy.m_contents.m_suspectid_size());
}

TEST_F(MessageSerilaizationTest, CONTROL_CLEAR_SUSPECT_REQUEST)
{
	// Create and serialize a message
	ControlMessage msg(CONTROL_CLEAR_SUSPECT_REQUEST);
	SuspectID_pb *temp = msg.m_contents.mutable_m_suspectid();
	temp->set_m_ip(42);
	temp->set_m_ifname("fake0");

	buffer = msg.Serialize(&bufferSize);

	// Deserialize it and make sure relevant internal variables for that message got copied okay
	// The 4 offset is hacky, but it's to strip off the message size that would normally be removed in Message::Deserialize
	ControlMessage deserializedCopy(buffer + 4, bufferSize);
	EXPECT_EQ(msg.m_contents.m_suspectid().SerializeAsString(), deserializedCopy.m_contents.m_suspectid().SerializeAsString());
}


TEST_F(MessageSerilaizationTest, UPDATE_SUSPECT_CLEARED)
{
	// Create and serialize a message
	UpdateMessage msg(UPDATE_SUSPECT_CLEARED);
	SuspectID_pb *temp = msg.m_contents.mutable_m_suspectid();
	temp->set_m_ip(42);
	temp->set_m_ifname("fake0");

	buffer = msg.Serialize(&bufferSize);

	// Deserialize it and make sure relevant internal variables for that message got copied okay
	// The 4 offset is hacky, but it's to strip off the message size that would normally be removed in Message::Deserialize
	UpdateMessage deserializedCopy(buffer + 4, bufferSize);
	EXPECT_EQ(msg.m_contents.m_suspectid().SerializeAsString(), deserializedCopy.m_contents.m_suspectid().SerializeAsString());
}


