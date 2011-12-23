/*
 * NOVAConfiguration.cpp
 *
 *  Created on: Dec 22, 2011
 *      Author: root
 */

#include "NOVAConfiguration.h"


namespace Nova {

NOVAConfiguration::NOVAConfiguration() {

}

void NOVAConfiguration::LoadConfig(char* input, string homePath) {
		string line;
		string prefix;

		cout << "Loading file " << input << " in homepath " << homePath << endl;

		ifstream config(input);

		const string prefixes[] = {"INTERFACE", "HS_HONEYD_CONFIG",
				"TCP_TIMEOUT","TCP_CHECK_FREQ",
				"READ_PCAP", "PCAP_FILE",
				"GO_TO_LIVE","USE_TERMINALS", "CLASSIFICATION_TIMEOUT"};

		if(config.is_open())
		{
			while(config.good())
			{
				getline(config,line);
				prefix = prefixes[0];

				cout << "Read in line " << line << endl;


				NovaOption * currentOption = new NovaOption();
				currentOption->isValid = false;


				if(!line.substr(0,prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size()+1,line.size());
					if(line.size() > 0)
					{
						// Try and detect a default interface by checking what the default route in the IP kernel's IP table is
						if (strcmp(line.c_str(), "default") == 0)
						{

							FILE * out = popen("netstat -rn","r");
							if(out != NULL)
							{
								char buffer[2048];
								char * column;
								int currentColumn;
								char * line = fgets(buffer, sizeof(buffer), out);

								while (line != NULL)
								{
									currentColumn = 0;
									column = strtok (line," \t\n");

									// Wait until we have the default route entry, ignore other entries
									if (strcmp(column, "0.0.0.0") == 0)
									{
										while (column != NULL)
										{
											// Get the column that has the interface name
											if (currentColumn == 7)
											{

												currentOption->data = column;
												// LOG4CXX_INFO(m_logger, "Interface Device defaulted to '" + dev + "'");
											}

											column = strtok (NULL, " \t\n");
											currentColumn++;
										}
									}

									line = fgets(buffer, sizeof(buffer), out);

								}
							}
							pclose(out);
						} else
							currentOption->data = line;

						if (!currentOption->data.empty())
							currentOption->isValid = true;
					}
					options.insert(pair<string, NovaOption>(prefix, *currentOption));
					continue;
				}
				prefix = prefixes[1];
				if(!line.substr(0,prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size()+1,line.size());
					if(line.size() > 0)
					{
						currentOption->data = homePath+"/"+line;
						currentOption->isValid = true;
					}
					options.insert(pair<string, NovaOption>(prefix, *currentOption));
					continue;
				}
				prefix = prefixes[2];
				if(!line.substr(0,prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size()+1,line.size());
					if(atoi(line.c_str()) > 0)
					{
						currentOption->data = line.c_str();
						currentOption->isValid = true;
					}
					options.insert(pair<string, NovaOption>(prefix, *currentOption));
					continue;
				}
				prefix = prefixes[3];
				if(!line.substr(0,prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size()+1,line.size());
					if(atoi(line.c_str()) > 0)
					{
						currentOption->data = line.c_str();
						currentOption->isValid = true;
					}
					options.insert(pair<string, NovaOption>(prefix, *currentOption));
					continue;
				}
				prefix = prefixes[4];
				if(!line.substr(0,prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size()+1,line.size());
					if(atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
					{
						currentOption->data = line.c_str();
						currentOption->isValid = true;
					}
					options.insert(pair<string, NovaOption>(prefix, *currentOption));
					continue;
				}
				prefix = prefixes[5];
				if(!line.substr(0,prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size()+1,line.size());
					if(line.size() > 0)
					{
						currentOption->data = homePath+"/"+line;
						currentOption->isValid = true;
					}
					options.insert(pair<string, NovaOption>(prefix, *currentOption));
					continue;
				}

				prefix = prefixes[6];
				if(!line.substr(0,prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size()+1,line.size());
					if(line.size() > 0)
					{
						currentOption->data = line.c_str();
						currentOption->isValid = true;
					}
					options.insert(pair<string, NovaOption>(prefix, *currentOption));
					continue;
				}

				prefix = prefixes[7];
				if(!line.substr(0,prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size()+1,line.size());
					if(atoi(line.c_str()) == 0 || atoi(line.c_str()) == 1)
					{
						currentOption->data = line.c_str();
						currentOption->isValid = true;
					}
					options.insert(pair<string, NovaOption>(prefix, *currentOption));
					continue;
				}

				prefix = prefixes[8];
				if(!line.substr(0,prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size()+1,line.size());
					if(atoi(line.c_str()) > 0)
					{
						currentOption->data = line.c_str();
						currentOption->isValid = true;
					}
					options.insert(pair<string, NovaOption>(prefix, *currentOption));
					continue;
				}
			}
		}
		else
		{
			//LOG4CXX_INFO(m_logger, "No configuration file detected.");
			exit(1);
		}
}

NOVAConfiguration::~NOVAConfiguration() {
}

}

