//============================================================================
// Name        : MessageManager.h
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
// Description : Manages all incoming messages on sockets
//============================================================================

#ifndef MESSAGEMANAGER_H_
#define MESSAGEMANAGER_H_

#include "messages/Message.h"
#include "MessageQueue.h"
#include "MessageEndpoint.h"
#include "MessageEndpointLock.h"
#include "../Lock.h"
#include "Ticket.h"

#include <map>
#include <vector>
#include "pthread.h"

namespace Nova
{

class MessageManager
{

public:

	static MessageManager &Instance();

	//Grabs a message off of the given socket and returns a pointer to it
	//	ticket - Ticket object holding all the necessary conversation info to pop a new message
	//	timeout - How long (in seconds) to wait for the message before giving up
	// Returns - A pointer to a valid Message object. Never NULL. Caller is responsible for life cycle of this message
	//		On error, this function returns an ErrorMessage with the details of the error
	//		IE: Returns ErrorMessage of type ERROR_TIMEOUT if timeout has been exceeded
	//	NOTE: You must have the lock on the socket by calling UseSocket() prior to calling this
	//		(Or bad things will happen)
	//	NOTE: Blocking function
	//	NOTE: Will automatically call CloseSocket() for you if the message returned happens to be an ERROR_MESSAGE
	//		of type ERROR_SOCKET_CLOSED. So there is no need to call it again yourself
	//	NOTE: Due to physical constraints, this function may block for longer than timeout. Don't rely on it being very precise.
	Nova::Message *ReadMessage(Ticket &ticket, int timeout = REPLY_TIMEOUT);

	//Writes a given Message to the provided socket
	//	ticket - Ticket object holding all the necessary conversation info to send this message
	//	message - A pointer to the message object to send
	// Returns - true on successfully sending the object, false on error
	bool WriteMessage(const Ticket &ticket, Message *message);

	//Informs the message manager that you would like to use the specified socket. Locks everyone else out from the socket.
	//	socketFD - The socket file descriptor to use
	//	returns - A Ticket object which contains all the information necessary to continue the
	//NOTE: Blocking function
	Ticket StartConversation(int socketFD);

	//Initializes the socket and its underlying MessageQueues. Should be called prior to other socket operations on this socketFD
	//	Failing to call StartSocket prior to use will cause you to get get ErrorMessges (but not crash)
	//TODO: Maybe make the other functions automatically check and call this function for us. So we can make this private
	//	socketFD - The socket file descriptor for which to make the initialization
	//NOTE: In order to use the returned MessageQueue safely, you must have a lock on it before calling this
	//NOTE: Safely does nothing if socketFD already exists in the manager
	void StartSocket(int socketFD);

	//Deletes the MessageQueue object to which socketFD belongs
	//	NOTE: Does not close the underlying socket. Use CloseSocket to do that.
	//	NOTE: Only called by callback thread
	void DeleteEndpoint(int socketFD);

	//Closes the socket at the given file descriptor
	//	socketFD - The file descriptor of the socket to close
	//	NOTE: This will not immediately destroy the underlying MessageQueue. It will close the socket
	//		such that no new messages can be read on it. At which point the read loop will mark the
	//		queue as closed with an ErrorMessage with the appropriate sub-type and then exit.
	//		The queue will not be actually destroyed until this last message is popped off.
	void CloseSocket(int socketFD);

	//Waits for a new callback message to arrive on the given socketFD
	//	socketFD - The socket file descriptor to wait on
	//	outTicket - Output parameter which provides all the information necessary to talk on the new callback conversation
	//	NOTE: Blocking call
	bool RegisterCallback(int socketFD, Ticket &outTicket);

	//Gets a current list of all the open sockets in the manager
	//	Returns - A vector of socket file descriptors
	//	NOTE: No locks are provided for these sockets. You're just given the descriptors. This necessarily means that by the time you
	//		get around to trying to read/write to the socket, it might have been closed. Or a new socket might have appeared that won't
	//		be included in this list. You'll just have to deal with this fact.
	std::vector <int>GetSocketList();

private:

	static MessageManager *m_instance;

	//Constructor for MessageManager
	MessageManager();

	//Function returns a read-locked MessageEndpoint
	//	Empty on error
	MessageEndpointLock GetEndpoint(int socketFD);

	std::map<int, std::pair<MessageEndpoint*, pthread_rwlock_t*>> m_endpoints;
	pthread_mutex_t m_endpointsMutex;

	pthread_mutex_t m_deleteEndpointMutex;;

};

}

#endif /* MESSAGEMANAGER_H_ */
