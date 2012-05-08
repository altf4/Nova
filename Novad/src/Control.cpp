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

#include "Doppelganger.h"
#include "Control.h"
#include "Config.h"
#include "Logger.h"
#include "Novad.h"

namespace Nova
{
void SaveAndExit(int param)
{
	if (Config::Inst()->GetIsTraining())
	{
		CloseTrainingCapture();
	}
	else
	{
		AppendToStateFile();
	}


	if(system("sudo iptables -F") == -1)
	{
		// TODO Logging
	}
	if(system("sudo iptables -t nat -F") == -1)
	{
		// TODO Logging
	}
	if(system("sudo iptables -t nat -X DOPP") == -1)
	{
		// TODO Logging
	}
	if(system(std::string("sudo route del " + Config::Inst()->GetDoppelIp()).c_str()) == -1)
	{
		// TODO Logging
	}
	LOG(NOTICE, "Novad is now exiting.", "");
	exit(EXIT_SUCCESS);
}
}
