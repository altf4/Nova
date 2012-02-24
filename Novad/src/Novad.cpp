//============================================================================
// Name        : Novad.cpp
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
// Description : Nova Daemon to perform network anti-reconnaissance
//============================================================================

#include "ClassificationEngine.h"
#include "Haystack.h"
#include "LocalTrafficMonitor.h"
#include "DoppelgangerModule.h"
#include "NovaUtil.h"
#include "NOVAConfiguration.h"

#include <iostream>

using namespace std;

string userHomePath, novaConfigPath;
NOVAConfiguration *globalConfig;

pthread_t CE_Thread, LTM_Thread, HS_Thread, DM_Thread;

int main()
{
	//TODO: Perhaps move this into its own init function?
	userHomePath = GetHomePath();
	novaConfigPath = userHomePath + "/Config/NOVAConfig.txt";

	globalConfig = new NOVAConfiguration(novaConfigPath);


	//Launch the 4 component threads
	pthread_create(&CE_Thread, NULL, ClassificationEngineMain, NULL);
	pthread_create(&LTM_Thread, NULL, LocalTrafficMonitorMain, NULL);
	pthread_create(&HS_Thread, NULL, HaystackMain, NULL);
	pthread_create(&DM_Thread, NULL, DoppelgangerModuleMain, NULL);


	return EXIT_FAILURE;
}
