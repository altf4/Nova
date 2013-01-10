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
	msg.m_suspectAddress.m_ip = 42;
	msg.m_suspectAddress.m_interface = "fake0";
	buffer = msg.Serialize(&bufferSize);

	// Deserialize it and make sure relevant internal variables for that message got copied okay
	// The 4 offset is hacky, but it's to strip off the message size that would normally be removed in Message::Deserialize
	RequestMessage deserializedCopy(buffer + 4, bufferSize);
	EXPECT_EQ(msg.m_suspectAddress, deserializedCopy.m_suspectAddress);
}

TEST_F(MessageSerilaizationTest, REQEST_SUSPECT_REPLY)
{
	RequestMessage msg(REQUEST_SUSPECT_REPLY);
}

TEST_F(MessageSerilaizationTest, REQUEST_SUSPECT_LIST)
{
	RequestMessage msg(REQUEST_SUSPECTLIST_REPLY);
	msg.m_suspectList.push_back(SuspectIdentifier(1, "fake1"));
	msg.m_suspectList.push_back(SuspectIdentifier(2, "fakeTestaoeuaoeuaoeu2"));
	msg.m_suspectList.push_back(SuspectIdentifier(3, "f3"));
	buffer = msg.Serialize(&bufferSize);

	RequestMessage deserializedCopy(buffer + 4, bufferSize);
	EXPECT_EQ(msg.m_suspectList.at(0), deserializedCopy.m_suspectList.at(0));
	EXPECT_EQ(msg.m_suspectList.at(1), deserializedCopy.m_suspectList.at(1));
	EXPECT_EQ(msg.m_suspectList.at(2), deserializedCopy.m_suspectList.at(2));
}

TEST_F(MessageSerilaizationTest, REQUEST_SUSPECT_LIST_empty)
{
	RequestMessage msg(REQUEST_SUSPECTLIST_REPLY);
	buffer = msg.Serialize(&bufferSize);

	RequestMessage deserializedCopy(buffer + 4, bufferSize);
	EXPECT_EQ(msg.m_suspectList.size(), deserializedCopy.m_suspectList.size());
}

TEST_F(MessageSerilaizationTest, CONTROL_CLEAR_SUSPECT_REQUEST)
{
	// Create and serialize a message
	ControlMessage msg(CONTROL_CLEAR_SUSPECT_REQUEST);
	msg.m_suspectAddress.m_ip = 42;
	msg.m_suspectAddress.m_interface = "fake0";
	buffer = msg.Serialize(&bufferSize);

	// Deserialize it and make sure relevant internal variables for that message got copied okay
	// The 4 offset is hacky, but it's to strip off the message size that would normally be removed in Message::Deserialize
	ControlMessage deserializedCopy(buffer + 4, bufferSize);
	EXPECT_EQ(msg.m_suspectAddress, deserializedCopy.m_suspectAddress);
}


TEST_F(MessageSerilaizationTest, UPDATE_SUSPECT_CLEARED)
{
	// Create and serialize a message
	UpdateMessage msg(UPDATE_SUSPECT_CLEARED);
	msg.m_IPAddress.m_ip = 42;
	msg.m_IPAddress.m_interface = "fake0";
	buffer = msg.Serialize(&bufferSize);

	// Deserialize it and make sure relevant internal variables for that message got copied okay
	// The 4 offset is hacky, but it's to strip off the message size that would normally be removed in Message::Deserialize
	UpdateMessage deserializedCopy(buffer + 4, bufferSize);
	EXPECT_EQ(msg.m_IPAddress, deserializedCopy.m_IPAddress);
}


