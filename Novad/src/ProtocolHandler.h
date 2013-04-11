//============================================================================
// Name        : ProtocolHandler.h
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
// Description : Manages the message sending protocol to and from the Nova UI
//============================================================================

#ifndef PROTOCOLHANDLER_H_
#define PROTOCOLHANDLER_H_

#include "messaging/Message.h"
#include "Suspect.h"

namespace Nova
{

void HandleExitRequest(Message *incoming);

void HandleClearAllRequest(Message *incoming);

void HandleClearSuspectRequest(Message *incoming);

void HandleSaveSuspectsRequest(Message *incoming);

void HandleReclassifyAllRequest(Message *incoming);

void HandleStartCaptureRequest(Message *incoming);

void HandleStopCaptureRequest(Message *incoming);

void HandleRequestSuspectList(Message *incoming);

void HandleRequestSuspect(Message *incoming);

void HandleRequestAllSuspects(Message *incoming);

void HandleRequestUptime(Message *incoming);

void HandlePing(Message *incoming);

}
#endif /* PROTOCOLHANDLER_H_ */
