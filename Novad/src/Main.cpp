//============================================================================
// Name        : Main.cpp
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
// Description : Wrapper function for the actual "main" to help in unit tests
//============================================================================

#include "Novad.h"
#include "Config.h"
#include "Logger.h"
#include <boost/program_options.hpp>
#include <stdio.h>
//define NC Config::Inst()//nova config static object

using namespace Nova;



int main(int argc, char ** argv)
{
	string pCAPFilePath;
	namespace po = boost::program_options;
	po::options_description desc("Command line options");
		try
		{
			desc.add_options()
					("help,h", "Show command line options")
					("pcap-file,p", po::value<string>(&pCAPFilePath), "specify Different Config Path");
			po::variables_map vm;
			po::store(po::parse_command_line(argc, argv, desc), vm);
			po::notify(vm);

			if(vm.count("help"))
			{
				std::cout << desc << std::endl;
				return 0;//should say return success
			}
			if(vm.count("pcap-file"))
			{
				Config::Inst()->SetPathPcapFile(pCAPFilePath);
				Config::Inst()->SetReadPcap(true);
				//change the configuration file path to the command line argument
			}
		}
			///usr/share/nova/userFiles/config/NOVAConfig.txt
		catch(exception &e)
				{
					LOG(ERROR, "Uncaught exception: " + string(e.what()) + ".", "");

					std::cout << '\n' << desc << std::endl;
				}



	return RunNovaD();
}
