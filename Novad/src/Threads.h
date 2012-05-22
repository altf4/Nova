//============================================================================
// Name        : Control.h
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
// Description : Novad thread loops
//============================================================================

#ifndef THREAD_H_
#define THREAD_H_

namespace Nova
{
// Start routine for a separate thread which infinite loops, periodically
// updating all the classifications for all the current suspects
//		prt - Required for pthread start routines
void *ClassificationLoop(void *ptr);


// Start routine for thread that calculates training data, and used for writing to file.
//		prt - Required for pthread start routines
void *TrainingLoop(void *ptr);


// Startup routine for thread that listens for Silent Alarms from other Nova instances
//		prt - Required for pthread start routines
void *SilentAlarmLoop(void *ptr);


// Updates the pcap filter based on DHCP addresses from Honeyd
void *UpdateIPFilter(void *ptr);

// Updates the pcap filter string based on changes in the whitelist file
void *UpdateWhitelistIPFilter(void *ptr);


// Startup rotuine for thread periodically checking for TCP timeout.
// IE: Not all TCP sessions get torn down properly. Sometimes they just end midstram
// This thread looks for old tcp sessions and declares them terminated
//		ptr - Required for pthread start routines
void *TCPTimeout( void *ptr );

}

#endif /* THREAD_H_ */
