//============================================================================
// Name        : Control.cpp
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
// Description : Set of functions Novad can call to perform tasks related to
//			controlling the Novad process and operation
//============================================================================

#include "Control.h"
#include "Logger.h"
#include "Novad.h"

void Nova::SaveAndExit(int param)
{
	AppendToStateFile();
	system("sudo iptables -F");
	system("sudo iptables -t nat -F");
	LOG(NOTICE, "Novad is now exiting.", "");
	exit(EXIT_SUCCESS);
}
