//============================================================================
// Name        : novagui.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : The main NovaGUI component, utilizes the auto-generated ui_novagui.h
//============================================================================/*

#include <sys/un.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <QtGui>
#include <QApplication>
#include "novagui.h"
#include "novaconfig.h"
#include "run_popup.h"
#include <sstream>
#include <QString>
#include <signal.h>
#include <QChar>
#include <fstream>
#include <log4cxx/xml/domconfigurator.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <errno.h>
#include <arpa/inet.h>
#include <math.h>

using namespace std;
using namespace Nova;
using namespace ClassificationEngine;
using namespace log4cxx;
using namespace log4cxx::xml;

int CE_InSock, CE_OutSock, DM_OutSock, HS_OutSock, LTM_OutSock;
struct sockaddr_un CE_InAddress, CE_OutAddress, DM_OutAddress, HS_OutAddress, LTM_OutAddress;
bool novaRunning = false;
bool useTerminals;

static SuspectHashTable SuspectTable;

//Variables for message sends.
const char* data;
int dataLen, len;

u_char buf[MAX_MSG_SIZE];
int bytesRead;

pthread_rwlock_t lock;

LoggerPtr m_logger(Logger::getLogger("main"));

char * pathsFile = (char*)"/etc/nova/paths";
string homePath, readPath, writePath;

/************************************************
 * Constructors, Destructors and Closing Actions
 ************************************************/

//Called when process receives a SIGINT, like if you press ctrl+c
void sighandler(int param)
{
	param = param;
	closeNova();
	exit(1);
}

NovaGUI::NovaGUI(QWidget *parent)
    : QMainWindow(parent)
{
	signal(SIGINT, sighandler);
	pthread_rwlock_init(&lock, NULL);
	ui.setupUi(this);
	runAsWindowUp = false;
	editingPreferences = false;

	getInfo();

	string novaConfig = "Config/NOVAConfig.txt";
	string logConfig = "Config/Log4cxxConfig_Console.xml";

	DOMConfigurator::configure(logConfig.c_str());

	loadAll();

	//Not sure why this is needed, but it seems to take care of the error
	// the abstracted Qt operations of QObject::connect sometimes throws an
	// error complaining about queueing objects of type 'QItemSelection'
	qRegisterMetaType<QItemSelection>("QItemSelection");

	//Sets up the socket addresses
	getSocketAddr();

	//Create listening socket, listen thread and draw thread --------------
	pthread_t CEListenThread;
	pthread_t CEDrawThread;

	if((CE_InSock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
		sclose(CE_InSock);
		exit(1);
	}

	len = strlen(CE_InAddress.sun_path) + sizeof(CE_InAddress.sun_family);

	if(bind(CE_InSock,(struct sockaddr *)&CE_InAddress,len) == -1)
	{
		LOG4CXX_ERROR(m_logger, "bind: " << strerror(errno));
		sclose(CE_InSock);
		exit(1);
	}

	if(listen(CE_InSock, SOCKET_QUEUE_SIZE) == -1)
	{
		LOG4CXX_ERROR(m_logger, "listen: " << strerror(errno));
		sclose(CE_InSock);
		exit(1);
	}

	//Sets initial view
	this->ui.stackedWidget->setCurrentIndex(0);
	this->ui.mainButton->setFlat(true);
	this->ui.suspectButton->setFlat(false);
	this->ui.doppelButton->setFlat(false);
	this->ui.haystackButton->setFlat(false);

	pthread_create(&CEListenThread,NULL,CEListen, this);
	pthread_create(&CEDrawThread,NULL,CEDraw, this);
}

NovaGUI::~NovaGUI()
{

}

void NovaGUI::closeEvent(QCloseEvent * e)
{
	e = e;
	closeNova();
	//this->close();
}

/************************************************
 * Gets preliminary information
 ************************************************/

void NovaGUI::getInfo()
{
	getPaths();
	getSettings();
}

void NovaGUI::getPaths()
{
	string line, prefix; //used for input checking

	//Get locations of nova files
	ifstream *paths =  new ifstream(pathsFile);

	if(paths->is_open())
	{
		while(paths->good())
		{
			getline(*paths,line);

			prefix = "NOVA_HOME";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				homePath = line;
				continue;
			}
			prefix = "NOVA_WR";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				writePath = line;
				continue;
			}
			prefix = "NOVA_RD";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				readPath = line;
				continue;
			}
		}
	}
	paths->close();
	delete paths;
	paths = NULL;

	//Resolves environment variables
	int start = 0;
	int end = 0;
	string var;

	while((start = homePath.find("$",end)) != -1)
	{
		end = homePath.find("/", start);
		//If no path after environment var
		if(end == -1)
		{

			var = homePath.substr(start+1, homePath.size());
			var = getenv((char*)var.c_str());
			homePath = homePath.substr(0,start) + var;
		}
		else
		{
			var = homePath.substr(start+1, end-1);
			var = getenv((char*)var.c_str());
			var = var + homePath.substr(end, homePath.size());
			if(start > 0)
			{
				homePath = homePath.substr(0,start)+var;
			}
			else
			{
				homePath = var;
			}
		}
	}

	start = 0;
	end = 0;
	while((start = readPath.find("$",end)) != -1)
	{
		end = readPath.find("/", start);
		//If no path after environment var
		if(end == -1)
		{

			var = readPath.substr(start+1, readPath.size());
			var = getenv((char*)var.c_str());
			readPath = readPath.substr(0,start) + var;
		}
		else
		{
			var = readPath.substr(start+1, end-1);
			var = getenv((char*)var.c_str());
			var = var + readPath.substr(end, readPath.size());
			if(start > 0)
			{
				readPath = readPath.substr(0,start)+var;
			}
			else
			{
				readPath = var;
			}
		}
	}

	start = 0;
	end = 0;
	while((start = writePath.find("$",end)) != -1)
	{
		end = writePath.find("/", start);
		//If no path after environment var
		if(end == -1)
		{

			var = writePath.substr(start+1, writePath.size());
			var = getenv((char*)var.c_str());
			writePath = writePath.substr(0,start) + var;
		}
		else
		{
			var = writePath.substr(start+1, end-1);
			var = getenv((char*)var.c_str());
			var = var + writePath.substr(end, writePath.size());
			if(start > 0)
			{
				writePath = writePath.substr(0,start)+var;
			}
			else
			{
				writePath = var;
			}
		}
	}

	if((homePath == "") || (readPath == "") || (writePath == ""))
	{
		exit(1);
	}

	QDir::setCurrent((QString)homePath.c_str());
}

void NovaGUI::getSettings()
{
	string line, prefix; //used for input checking

	//Get locations of nova files
	ifstream *settings =  new ifstream((homePath+"/settings").c_str());

	if(settings->is_open())
	{
		while(settings->good())
		{
			getline(*settings,line);
			prefix = "group";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				group = line;
				continue;
			}
			/*prefix = "subnet";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				mainSubnet.numMaskBits = atoi(line.substr(line.find("/")+1,(line.size()-(line.find("/")+1))).c_str());
				mainSubnet.address = line;
				mainSubnet.base = htonl(inet_addr(line.substr(0,line.find("/")).c_str()));
				uint tempBase = 0;
				uint tempRange = (pow(2, 32) - 1);
				for(int i = 0; i < mainSubnet.numMaskBits; i ++)
				{
					tempBase += pow(2, 31-i);
					tempRange -= pow(2, 31-i);
				}
				mainSubnet.base &= tempBase;
				mainSubnet.range = mainSubnet.base+tempRange;
				mainSubnet.enabled = true;
				continue;
			}*/
		}
	}
	//subnets[mainSubnet.address] = mainSubnet;
	settings->close();
	delete settings;
	settings = NULL;
}

/************************************************
 * Thread Loops
 ************************************************/

void *CEListen(void *ptr)
{
	while(true)
	{
		((NovaGUI*)ptr)->receiveCE(CE_InSock);
	}
	return NULL;
}

void *CEDraw(void *ptr)
{
	while(true)
	{
		((NovaGUI*)ptr)->drawSuspects();
		sleep(5);
	}
	return NULL;
}

/************************************************
 * Load Honeyd XML Configuration Functions
 ************************************************/

//Calls all load functions
void NovaGUI::loadAll()
{
	loadScripts();
	loadPorts();
	loadProfiles();
	loadGroup();
}

//Loads scripts from file
void NovaGUI::loadScripts()
{
	using boost::property_tree::ptree;
	scriptTree.clear();
	try
	{
		read_xml(homePath+"/scripts.xml", scriptTree);

		BOOST_FOREACH(ptree::value_type &v, scriptTree.get_child("scripts"))
		{
			script s;
			s.treePtr = &v.second;
			//Each script consists of a name and path to that script
			s.name = v.second.get<std::string>("name");
			s.path = v.second.get<std::string>("path");
			scripts[s.name] = s;
		}
	}
	catch(std::exception &e)
	{
		LOG4CXX_ERROR(m_logger, "Problem loading scripts: "+ string(e.what()));
	}
}

//Loads ports from file
void NovaGUI::loadPorts()
{
	using boost::property_tree::ptree;
	portTree.clear();
	try
	{
		read_xml(homePath+"/templates/ports.xml", portTree);

		BOOST_FOREACH(ptree::value_type &v, portTree.get_child("ports"))
		{
			port p;
			p.treePtr = &v.second;
			//Required xml entries
			p.portName = v.second.get<std::string>("name");
			p.portNum = v.second.get<std::string>("number");
			p.type = v.second.get<std::string>("type");
			p.behavior = v.second.get<std::string>("behavior");

			//If this port uses a script, find and assign it.
			if(!p.behavior.compare("script") || !p.behavior.compare("internal"))
			{
				p.scriptPtr = &scripts[v.second.get<std::string>("script")];
				p.scriptName = p.scriptPtr->name;
			}
			//If the port works as a proxy, find destination
			else if(!p.behavior.compare("proxy"))
			{
				p.proxyIP = v.second.get<std::string>("IP");
				p.proxyPort = v.second.get<std::string>("Port");
			}
			ports[p.portName] = p;
		}
	}
	catch(std::exception &e)
	{
		LOG4CXX_ERROR(m_logger, "Problem loading ports: "+ string(e.what()));
	}
}


//Loads the subnets and nodes from file for the currently specified group
void NovaGUI::loadGroup()
{
	using boost::property_tree::ptree;
	groupTree.clear();
	ptree ptr;

	try
	{
		read_xml(homePath+"/templates/nodes.xml", groupTree);
		BOOST_FOREACH(ptree::value_type &v, groupTree.get_child("groups"))
		{
			//Find the specified group
			if(!v.second.get<std::string>("name").compare(group))
			{
				try //Null Check
				{
					//Load Subnets first, they are needed before we can load nodes
					subnetTree = &v.second.get_child("subnets");
					loadSubnets(subnetTree);

					try //Null Check
					{
						//If subnets are loaded successfully, load nodes
						nodesTree = &v.second.get_child("nodes");
						loadNodes(nodesTree);
					}
					catch(std::exception &e)
					{
						LOG4CXX_ERROR(m_logger, "Problem loading nodes: " + string(e.what()));
					}
				}
				catch(std::exception &e)
				{
					LOG4CXX_ERROR(m_logger, "Problem loading subnets: " + string(e.what()));
				}

				try //Null Check
				{
					//Loads doppelganger, doesn't have a subnet
					doppTree = &v.second.get_child("doppelganger");
					loadDoppelganger(doppTree);
				}
				catch(std::exception &e)
				{
					LOG4CXX_ERROR(m_logger, "Problem loading doppelganger: " + string(e.what()));
				}
			}
		}
	}
	catch(std::exception &e)
	{
		LOG4CXX_ERROR(m_logger, "Problem loading group: " + group + " - "+ string(e.what()));
	}
}

//loads subnets from file for current group
void NovaGUI::loadSubnets(ptree *ptr)
{
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			//If real interface
			if(!string(v.first.data()).compare("interface"))
			{
				subnet sub;
				sub.treePtr = &v.second;
				//Extract the data
				sub.name = v.second.get<std::string>("name");
				sub.address = v.second.get<std::string>("IP");
				sub.mask = v.second.get<std::string>("mask");

				//Gets the IP address in uint32 form
				in_addr_t baseTemp = htonl(inet_addr(sub.address.c_str()));

				//Converting the mask to uint32 allows a simple bitwise AND to get the lowest IP in the subnet.
				in_addr_t maskTemp = htonl(inet_addr(sub.mask.c_str()));
				sub.base = (baseTemp & maskTemp);
				//Get the number of bits in the mask
				sub.maskBits = getMaskBits(maskTemp);
				//Adding the binary inversion of the mask gets the highest usable IP
				sub.max = sub.base + ~maskTemp;

				//Save subnet
				subnets[sub.address] = sub;
			}
			//If virtual honeyd subnet
			else if(!string(v.first.data()).compare("virtual"))
			{
				//TODO
			}
			else
			{
				LOG4CXX_INFO(m_logger, "Unexpected Entry in file: "+ string(v.first.data()));
			}
		}
	}
	catch(std::exception &e)
	{
		LOG4CXX_ERROR(m_logger, "Problem loading subnets: "+ string(e.what()));
	}
}

//loads doppelganger from file for current group
void NovaGUI::loadDoppelganger(ptree *ptr)
{
	profile p;
	ptree *ptr2;
	try
	{
		//required values
		dm.treePtr = ptr;
		dm.interface = string(ptr->get<std::string>("interface"));
		dm.address = string(ptr->get<std::string>("IP"));
		dm.enabled = atoi(string(ptr->get<std::string>("enabled")).c_str());
		dm.pname = string(ptr->get<std::string>("profile.name"));
		dm.pfile = &profiles[dm.pname];

		try //Conditional: has "set" values
		{
			//Some values are set for the doppelganger specifically
			ptr2 = &ptr->get_child("profile.set");

			//Inherit all of parent profile's values
			p = *dm.pfile;
			//Create unique profile for the doppelganger
			p.name = dm.address;
			dm.pname = dm.address;
			p.parentProfile = dm.pfile;
			loadProfileSet(ptr2, &p);
		}
		catch(...){}

		try //Conditional: has "add" values
		{
			//Some ports or subsystems are specific to the doppelganger
			ptr2 = &ptr->get_child("profile.add");

			//If doppelganger already has a unique profile
			if(!dm.pname.compare(dm.address))
			{
				loadProfileAdd(ptr2, &p);
			}
			//If doppelganger needs a unique profile
			else
			{
				//Inherit all of parent profile's values
				p = *dm.pfile;
				//Create unique profile for the doppelganger
				p.name = dm.address;
				dm.pname = dm.address;
				p.parentProfile = dm.pfile;
				loadProfileAdd(ptr2, &p);
			}
		}
		catch(...){}
	}
	catch(std::exception &e)
	{
		LOG4CXX_ERROR(m_logger, "Problem loading doppelganger: "+ string(e.what()));
	}
}

//loads haystack nodes from file for current group
void NovaGUI::loadNodes(ptree *ptr)
{
	ptree * ptr2;
	profile p;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			if(!string(v.first.data()).compare("node"))
			{
				node n;
				n.treePtr = &v.second;
				//Required xml entires
				n.interface = v.second.get<std::string>("interface");
				n.address = v.second.get<std::string>("IP");
				n.enabled = atoi(v.second.get<std::string>("enabled").c_str());
				n.pname = v.second.get<std::string>("profile.name");

				//Looks up profile, these must be loaded first
				n.pfile = &profiles[n.pname];

				//intialize subnet to NULL and check for smallest bounding subnet
				n.sub = NULL;

				n.realIP = htonl(inet_addr(n.address.c_str())); //convert ip to uint32
				int max = 0; //Tracks the mask with smallest range by comparing num of bits used.

				//Check each subnet
				for(SubnetTable::iterator it = subnets.begin(); it != subnets.end(); it++)
				{
					//If node falls outside a subnets range skip it
					if((n.realIP < it->second.base) || (n.realIP > it->second.max))
						continue;
					//If this is the smallest range
					if(it->second.maskBits > max)
					{
						//If node isn't using host's address
						if(it->second.address.compare(n.address))
						{
							max = it->second.maskBits;
							n.sub = &it->second;
						}
					}
				}
				try //Conditional: has "set" values
				{
					//Some values are set for the node specifically
					ptr2 = &v.second.get_child("profile.set");

					//Inherit all of parent profile's values
					p = *n.pfile;
					//Create unique profile for this node
					p.name = n.address;
					n.pname = n.address;
					p.parentProfile = n.pfile;
					loadProfileSet(ptr2, &p);
				}
				catch(...){}

				try //Conditional: has "add" values
				{
					//Some ports or subsystems are specific to the node
					ptr2 = &v.second.get_child("profile.add");

					//If node already has a unique profile
					if(!n.pname.compare(n.address))
					{
						loadProfileAdd(ptr2, &p);
					}
					//If node needs a unique profile
					else
					{
						//Inherit all of parent profile's values
						p = *n.pfile;
						//Create unique profile for this node
						p.name = n.address;
						n.pname = n.address;
						p.parentProfile = n.pfile;
						loadProfileAdd(ptr2, &p);
					}
				}
				catch(...){}

				bool unique = true;
				//Check that node has unique IP addr
				for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
				{
					if(n.realIP == it->second.realIP)
					{
						unique = false;
					}
				}

				//If we have a subnet and node is unique
				if((n.sub != NULL) && unique)
				{
					//If node has unique profile, include it in the profiles table
					if(!n.pname.compare(n.address))
					{
						profiles[n.pname] = p;
						n.pfile = &profiles[n.pname];
					}

					//save the node in the table
					nodes[n.address] = n;
					//Put address of saved node in subnet's list of nodes.
					nodes[n.address].sub->nodes.push_back(&nodes[n.address]);
				}
				//If no subnet found, can't use node.
				else
				{
					LOG4CXX_ERROR(m_logger, "Node at IP: " + n.address + " is outside all valid subnet ranges");
				}
			}
			else
			{
				LOG4CXX_INFO(m_logger, "Unexpected Entry in file: "+ string(v.first.data()));
			}
		}
	}
	catch(std::exception &e)
	{
		LOG4CXX_ERROR(m_logger, "Problem loading nodes: "+ string(e.what()));
	}
}
void NovaGUI::loadProfiles()
{
	using boost::property_tree::ptree;
	ptree * ptr;
	profileTree.clear();
	try
	{
		read_xml(homePath+"/templates/profiles.xml", profileTree);

		BOOST_FOREACH(ptree::value_type &v, profileTree.get_child("profiles"))
		{
			//Generic profile, essentially a honeyd template
			if(!string(v.first.data()).compare("profile"))
			{
				profile p;
				//Root profile has no parent
				p.parentProfile = NULL;
				p.ptreePtr = &v.second;

				//Name required, DCHP boolean intialized (set in loadProfileSet)
				p.name = v.second.get<std::string>("name");
				p.DHCP = false;
				p.ports.clear();

				try //Conditional: has "set" values
				{
					ptr = &v.second.get_child("set");
					//pass 'set' subset and pointer to this profile
					loadProfileSet(ptr, &p);
				}
				catch(...){}

				try //Conditional: has "add" values
				{
					ptr = &v.second.get_child("add");
					//pass 'add' subset and pointer to this profile
					loadProfileAdd(ptr, &p);
				}
				catch(...){}

				//Save the profile
				profiles[p.name] = p;

				try //Conditional: has children profiles
				{
					ptr = &v.second.get_child("profiles");

					//start recurisive descent down profile tree with this profile as the root
					//pass subtree and pointer to parent
					loadSubProfiles(ptr, &profiles[p.name]);
				}
				catch(...){}

			}

			//Honeyd's implementation of switching templates based on conditions
			else if(!string(v.first.data()).compare("dynamic"))
			{
				//TODO
			}
			else
			{
				LOG4CXX_ERROR(m_logger, "Invalid XML Path" +string(v.first.data()));
			}
		}
	}
	catch(std::exception &e)
	{
		LOG4CXX_ERROR(m_logger, "Problem loading Profiles: "+ string(e.what()));
	}
}

//Sets the configuration of 'set' values for profile that called it
void NovaGUI::loadProfileSet(ptree *ptr, profile *p)
{
	string prefix;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			prefix = "TCP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->tcpAction = v.second.data();
				continue;
			}
			prefix = "UDP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->udpAction = v.second.data();
				continue;
			}
			prefix = "ICMP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->icmpAction = v.second.data();
				continue;
			}
			prefix = "personality";
			if(!string(v.first.data()).compare(prefix))
			{
				p->personality = v.second.data();
				continue;
			}
			prefix = "ethernet";
			if(!string(v.first.data()).compare(prefix))
			{
				p->ethernet = v.second.data();
				continue;
			}
			prefix = "uptime";
			if(!string(v.first.data()).compare(prefix))
			{
				p->uptime = v.second.data();
				continue;
			}
			prefix = "uptimeRange";
			if(!string(v.first.data()).compare(prefix))
			{
				p->uptimeRange = v.second.data();
				continue;
			}
			prefix = "dropRate";
			if(!string(v.first.data()).compare(prefix))
			{
				p->dropRate = v.second.data();
				continue;
			}
			prefix = "DHCP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->DHCP = true;
				continue;
			}
		}
	}
	catch(std::exception &e)
	{
		LOG4CXX_ERROR(m_logger, "Problem loading profile set parameters: "+ string(e.what()));
	}
}

//Adds specified ports and subsystems
// removes any previous port with same number and type to avoid conflicts
void NovaGUI::loadProfileAdd(ptree *ptr, profile *p)
{
	string prefix;
	port * prt;

	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			//Checks for ports
			prefix = "ports";
			if(!string(v.first.data()).compare(prefix))
			{
				//Iterates through the ports
				BOOST_FOREACH(ptree::value_type &v2, ptr->get_child("ports"))
				{
					prt = &ports[v2.second.data()];

					//Checks inherited ports for conflicts
					for(uint i = 0; i < p->ports.size(); i++)
					{
						//Erase inherited port if a conflict is found
						if(!prt->portNum.compare(p->ports[i].portNum) && !prt->type.compare(p->ports[i].type))
						{
							p->ports.erase(p->ports.begin()+i);
						}
					}
					//Add specified port
					p->ports.push_back(*prt);
				}
				continue;
			}

			//Checks for a subsystem
			prefix = "subsystem"; //TODO
			if(!string(v.first.data()).compare(prefix))
			{
				continue;
			}
		}
	}
	catch(std::exception &e)
	{
		LOG4CXX_ERROR(m_logger, "Problem loading profile add parameters: "+ string(e.what()));
	}
}

//Recurisve descent down a profile tree, inherits parent, sets values and continues if not leaf.
void NovaGUI::loadSubProfiles(ptree *ptr, profile * p)
{
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			ptree *ptr2;

			//Inherits parent,
			profile prof = *p;
			prof.ptreePtr = &v.second;
			prof.parentProfile = p;

			//Gets name, initializes DHCP
			prof.name = v.second.get<std::string>("name");
			prof.DHCP = false;

			try //Conditional: If profile has set configurations different from parent
			{
				ptr2 = &v.second.get_child("set");
				loadProfileSet(ptr2, &prof);
			}
			catch(...){}

			try //Conditional: If profile has port or subsystems different from parent
			{
				ptr2 = &v.second.get_child("add");
				loadProfileAdd(ptr2, &prof);
			}
			catch(...){}

			//Saves the profile
			profiles[prof.name] = prof;


			try //Conditional: if profile has children (not leaf)
			{
				ptr2 = &v.second.get_child("profiles");
				loadSubProfiles(ptr2, &profiles[prof.name]);
			}
			catch(...){}
		}
	}
	catch(std::exception &e)
	{
		LOG4CXX_ERROR(m_logger, "Problem loading sub profiles: "+ string(e.what()));
	}
}


/************************************************
 * Suspect Functions
 ************************************************/

bool NovaGUI::receiveCE(int socket)
{
	struct sockaddr_un remote;
	int socketSize, connectionSocket;
	socketSize = sizeof(remote);

	//Blocking call
	if ((connectionSocket = accept(socket, (struct sockaddr *)&remote, (socklen_t*)&socketSize)) == -1)
	{
		LOG4CXX_ERROR(m_logger, "accept: " << strerror(errno));
		sclose(connectionSocket);
		return false;
	}
	if((bytesRead = recv(connectionSocket, buf, MAX_MSG_SIZE, 0 )) == 0)
	{
		return false;
	}
	else if(bytesRead == -1)
	{
		LOG4CXX_ERROR(m_logger, "recv: " << strerror(errno));
		sclose(connectionSocket);
		return false;
	}
	sclose(connectionSocket);

	Suspect* suspect = new Suspect();

	try
	{
		suspect->deserializeSuspect(buf);
		bzero(buf, bytesRead);
	}
	catch(std::exception e)
	{
		LOG4CXX_ERROR(m_logger, "Error interpreting received Suspect: " << string(e.what()));
		delete suspect;
		return false;
	}

	struct suspectItem sItem;
	sItem.suspect = suspect;
	sItem.item = NULL;
	sItem.mainItem = NULL;
	updateSuspect(sItem);
	return true;
}

void NovaGUI::updateSuspect(suspectItem suspectItem)
{

	pthread_rwlock_wrlock(&lock);

	//We borrow the flag to show there is new information.
	suspectItem.suspect->needs_feature_update = true;

	//If the suspect already exists in our table
	if(SuspectTable.find(suspectItem.suspect->IP_address.s_addr) != SuspectTable.end())
	{
		//If classification hasn't changed
		if(suspectItem.suspect->isHostile == SuspectTable[suspectItem.suspect->IP_address.s_addr].suspect->isHostile)
		{
			//We set this flag if the QWidgetListItem needs to be changed or created
			//We check for a NULL item so that if the suspect list is cleared but the classification hasn't changed
			//We still create a new item during drawSuspects
			if(SuspectTable[suspectItem.suspect->IP_address.s_addr].item != NULL)
			{
				suspectItem.suspect->needs_classification_update = false;
			}
			else
			{
				suspectItem.suspect->needs_classification_update = true;
			}
		}

		//If it has
		else
		{
			//This flag is set because the classification has changed, and the item needs to be moved
			suspectItem.suspect->needs_classification_update = true;
		}

		//Delete the old Suspect since we created and pointed to a new one
		delete SuspectTable[suspectItem.suspect->IP_address.s_addr].suspect;

		//We point to the same item so it doesn't need to be deleted.
		suspectItem.item = SuspectTable[suspectItem.suspect->IP_address.s_addr].item;
		suspectItem.mainItem = SuspectTable[suspectItem.suspect->IP_address.s_addr].mainItem;

		//Update the entry in the table
		SuspectTable[suspectItem.suspect->IP_address.s_addr] = suspectItem;
	}
	//If the suspect is new
	else
	{
		//This flag is set because it is a new item and to the GUI display has no classification yet
		suspectItem.suspect->needs_classification_update = true;
		suspectItem.item = NULL;
		suspectItem.mainItem = NULL;

		//Put it in the table
		SuspectTable[suspectItem.suspect->IP_address.s_addr] = suspectItem;
	}
	pthread_rwlock_unlock(&lock);
}

/************************************************
 * Display Functions
 ************************************************/

void NovaGUI::drawAllSuspects()
{
	clearSuspectList();
	//Create the colors for the draw
	QBrush gbrush(QColor(0, 150, 0, 255));
	gbrush.setStyle(Qt::NoBrush);
	QBrush rbrush(QColor(200, 0, 0, 255));
	rbrush.setStyle(Qt::NoBrush);

	QListWidgetItem * item = NULL;
	QListWidgetItem * mainItem = NULL;
	Suspect * suspect = NULL;
	QString str;

	pthread_rwlock_wrlock(&lock);
	for (SuspectHashTable::iterator it = SuspectTable.begin() ; it != SuspectTable.end(); it++)
	{
		str = (QString)it->second.suspect->ToString().c_str();

		suspect = it->second.suspect;
		//If Benign
		if(suspect->isHostile == false)
		{
			//Create the Suspect in list with info set alignment and color
			item = new QListWidgetItem(str, this->ui.benignList, 0);
			item->setTextAlignment(Qt::AlignLeft|Qt::AlignBottom);
			item->setForeground(gbrush);
			//Copy the item
			mainItem = new QListWidgetItem(*item);
			//Point to the new items
			it->second.mainItem = mainItem;
			it->second.item = item;
		}
		//If Hostile
		else
		{
			//Create the Suspect in list with info set alignment and color
			item = new QListWidgetItem(str, this->ui.hostileList, 0);
			item->setTextAlignment(Qt::AlignLeft|Qt::AlignBottom);
			item->setForeground(rbrush);
			//Copy the item and add it to the list
			mainItem = new QListWidgetItem(*item);
			ui.mainSuspectList->addItem(mainItem);
			//Point to the new items
			it->second.item = item;
			it->second.mainItem = mainItem;
		}
		//Reset the flags
		suspect->needs_feature_update = false;
		suspect->needs_classification_update = false;
		it->second.suspect = suspect;
	}
	pthread_rwlock_unlock(&lock);
}

void NovaGUI::drawSuspects()
{
	//Create the colors for the draw
	QBrush gbrush(QColor(0, 150, 0, 255));
	gbrush.setStyle(Qt::NoBrush);
	QBrush rbrush(QColor(200, 0, 0, 255));
	rbrush.setStyle(Qt::NoBrush);

	QListWidgetItem * item = NULL;
	QListWidgetItem * mainItem = NULL;
	Suspect * suspect = NULL;
	QString str;

	pthread_rwlock_wrlock(&lock);
	for (SuspectHashTable::iterator it = SuspectTable.begin() ; it != SuspectTable.end(); it++)
	{
		//If there is new information
		if(it->second.suspect->needs_feature_update)
		{
			//Extract Information
			str = (QString)it->second.suspect->ToString().c_str();
			//Set pointers for fast access
			item = it->second.item;
			mainItem = it->second.mainItem;
			suspect = it->second.suspect;

			//If the item exists and is in the same list
			if(suspect->needs_classification_update == false)
			{
				item->setText(str);
				mainItem->setText(str);
				//Reset the flag
				suspect->needs_feature_update = false;
			}
			//If the item exists but classification has changed
			else if(item != NULL)
			{
				//If Benign, old item is in hostile
				if(suspect->isHostile == false)
				{
					//Remove old item, update info, change color, put in new list
					this->ui.hostileList->removeItemWidget(item);
					this->ui.mainSuspectList->removeItemWidget(mainItem);
					item->setForeground(gbrush);
					item->setText(str);
					mainItem->setText(str);
					this->ui.benignList->addItem(item);
				}
				//If Hostile, old item is in benign
				else
				{
					//Remove old item, update info, change color, put in new list
					this->ui.benignList->removeItemWidget(item);
					item->setForeground(rbrush);
					item->setText(str);
					mainItem->setForeground(rbrush);
					mainItem->setText(str);
					this->ui.hostileList->addItem(item);
					this->ui.mainSuspectList->addItem(mainItem);
				}
				//Reset the flags
				suspect->needs_feature_update = false;
				suspect->needs_classification_update = false;
			}
			//If it's a new item
			else
			{
				//If Benign
				if(suspect->isHostile == false)
				{
					//Create the Suspect in list with info set alignment and color
					item = new QListWidgetItem(str, this->ui.benignList, 0);
					item->setTextAlignment(Qt::AlignLeft|Qt::AlignBottom);
					item->setForeground(gbrush);
					//Copy the item
					mainItem = new QListWidgetItem(*item);
					//Point to the new items
					it->second.mainItem = mainItem;
					it->second.item = item;
				}
				//If Hostile
				else
				{
					//Create the Suspect in list with info set alignment and color
					item = new QListWidgetItem(str, this->ui.hostileList, 0);
					item->setTextAlignment(Qt::AlignLeft|Qt::AlignBottom);
					item->setForeground(rbrush);
					//Copy the item and add it to the list
					mainItem = new QListWidgetItem(*item);
					ui.mainSuspectList->addItem(mainItem);
					//Point to the new items
					it->second.item = item;
					it->second.mainItem = mainItem;
				}
				//Reset the flags
				suspect->needs_feature_update = false;
				suspect->needs_classification_update = false;
			}
		}
	}
	pthread_rwlock_unlock(&lock);
}

void NovaGUI::clearSuspectList()
{
	pthread_rwlock_wrlock(&lock);
	this->ui.mainSuspectList->clear();
	this->ui.hostileList->clear();
	this->ui.benignList->clear();
	//Since clearing permenantly deletes the items we need to make sure the suspects point to null
	for (SuspectHashTable::iterator it = SuspectTable.begin() ; it != SuspectTable.end(); it++)
	{
		it->second.mainItem = NULL;
		it->second.item = NULL;
	}
	pthread_rwlock_unlock(&lock);
}

void NovaGUI::drawNodes()
{
	QTreeWidgetItem * item = NULL;
	QString str;

	ui.nodesTreeWidget->clear();

	for(SubnetTable::iterator it = subnets.begin(); it != subnets.end(); it++)
	{
		item = new QTreeWidgetItem(ui.nodesTreeWidget);
		str = (QString)it->second.address.c_str();
	}
	for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
	{

	}
}

/************************************************
 * Menu Signal Handlers
 ************************************************/

void NovaGUI::on_actionRunNova_triggered()
{
	startNova();
}

void NovaGUI::on_actionRunNovaAs_triggered()
{
	if(!runAsWindowUp && !novaRunning)
	{
		Run_Popup *w = new Run_Popup(this, homePath);
		w->show();
		runAsWindowUp = true;
	}
}

void NovaGUI::on_actionStopNova_triggered()
{
	closeNova();
}

void NovaGUI::on_actionConfigure_triggered()
{
	if(!editingPreferences)
	{
		NovaConfig *w = new NovaConfig(this, homePath);
		w->show();
		editingPreferences = true;
	}
}

void  NovaGUI::on_actionExit_triggered()
{
	closeNova();
	exit(1);
}

void  NovaGUI::on_actionHide_Old_Suspects_triggered()
{
	clearSuspectList();
}

void  NovaGUI::on_actionShow_All_Suspects_triggered()
{
	drawAllSuspects();
}

/************************************************
 * View Signal Handlers
 ************************************************/

void NovaGUI::on_mainButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(0);
	this->ui.mainButton->setFlat(true);
	this->ui.suspectButton->setFlat(false);
	this->ui.doppelButton->setFlat(false);
	this->ui.haystackButton->setFlat(false);
}

void NovaGUI::on_suspectButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(1);
	this->ui.mainButton->setFlat(false);
	this->ui.suspectButton->setFlat(true);
	this->ui.doppelButton->setFlat(false);
	this->ui.haystackButton->setFlat(false);
}

void NovaGUI::on_doppelButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(2);
	this->ui.mainButton->setFlat(false);
	this->ui.suspectButton->setFlat(false);
	this->ui.doppelButton->setFlat(true);
	this->ui.haystackButton->setFlat(false);
}

void NovaGUI::on_haystackButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(3);
	this->ui.mainButton->setFlat(false);
	this->ui.suspectButton->setFlat(false);
	this->ui.doppelButton->setFlat(false);
	this->ui.haystackButton->setFlat(true);
}

/************************************************
 * Button Signal Handlers
 ************************************************/

void  NovaGUI::on_runButton_clicked()
{
	startNova();
}
void  NovaGUI::on_stopButton_clicked()
{
	closeNova();
}
void  NovaGUI::on_clearSuspectsButton_clicked()
{
	clearSuspects();
	drawAllSuspects();
}

/************************************************
 * IPC Functions
 ************************************************/

void clearSuspects()
{
	pthread_rwlock_wrlock(&lock);
	SuspectTable.clear();
	data = "CLEAR ALL";
	dataLen = string(data).size();
	sendToCE();
	sendToDM();
	pthread_rwlock_unlock(&lock);
}

void closeNova()
{
	if(novaRunning)
	{
		//Sets the message
		data = "EXIT";
		dataLen = string(data).size();

		//Sends the message to all Nova processes
		sendAll();

		// Close Honeyd processes
		FILE * out = popen("pidof honeyd","r");
		if(out != NULL)
		{
			char buffer[1024];
			char * line = fgets(buffer, sizeof(buffer), out);
			string cmd = "kill " + string(line);
			system(cmd.c_str());
		}
		pclose(out);
		novaRunning = false;
	}
}

void startNova()
{
	if(!novaRunning)
	{
		string path = getenv("HOME");
		path += "/.nova/Config/NOVAConfig.txt";
		ifstream * config = new ifstream((char*)path.c_str());
		if(config->is_open())
		{
			string line, prefix;
			prefix = "USE_TERMINALS";
			while(config->good())
			{
				getline(*config, line);
				if(!line.substr(0,prefix.size()).compare(prefix))
				{
					line = line.substr(prefix.size()+1,line.size());
					useTerminals = atoi(line.c_str());
					continue;
				}
			}
		}
		config->close();
		delete config;

		if(!useTerminals)
		{
			system(("nohup honeyd -i eth0 -f "+homePath+"/Config/haystack.config -p "+readPath+"/nmap-os-fingerprints"
					" -s "+writePath+"/Logs/honeydservice.log > /dev/null &").c_str());
			system(("nohup honeyd -i lo -f "+homePath+"/Config/doppelganger.config -p "+readPath+"/nmap-os-fingerprints"
					" -s "+writePath+"/Logs/honeydDoppservice.log 10.0.0.0/8 > /dev/null &").c_str());
			system("nohup LocalTrafficMonitor > /dev/null &");
			system("nohup Haystack > /dev/null &");
			system("nohup ClassificationEngine > /dev/null &");
			system("nohup DoppelgangerModule > /dev/null &");
		}
		else
		{
			system(("(gnome-terminal -t \"HoneyD Haystack\" --geometry \"+0+0\" -x honeyd -d -i eth0 -f "+homePath+"/Config/haystack.config"
					" -p "+readPath+"/nmap-os-fingerprints -s "+writePath+"/Logs/honeydservice.log )&").c_str());
			system(("(gnome-terminal -t \"HoneyD Doppelganger\" --geometry \"+500+0\" -x honeyd -d -i lo -f "+homePath+"/Config/doppelganger.config"
					" -p "+readPath+"/nmap-os-fingerprints -s "+writePath+"/Logs/honeydDoppservice.log 10.0.0.0/8 )&").c_str());
			system("(gnome-terminal -t \"LocalTrafficMonitor\" --geometry \"+1000+0\" -x LocalTrafficMonitor)&");
			system("(gnome-terminal -t \"Haystack\" --geometry \"+1000+600\" -x Haystack)&");
			system("(gnome-terminal -t \"ClassificationEngine\" --geometry \"+0+600\" -x ClassificationEngine)&");
			system("(gnome-terminal -t \"DoppelgangerModule\" --geometry \"+500+600\" -x DoppelgangerModule)&");
		}
		novaRunning = true;
	}
}

/************************************************
 * Socket Functions
 ************************************************/

void getSocketAddr()
{

	//CE IN --------------------------------------------------
	//Builds the key path
	string key = CE_FILENAME;
	key = homePath + key;
	//Builds the address
	CE_InAddress.sun_family = AF_UNIX;
	strcpy(CE_InAddress.sun_path, key.c_str());
	unlink(CE_InAddress.sun_path);

	//CE OUT -------------------------------------------------
	//Builds the key path
	key = CE_GUI_FILENAME;
	key = homePath + key;
	//Builds the address
	CE_OutAddress.sun_family = AF_UNIX;
	strcpy(CE_OutAddress.sun_path, key.c_str());

	//DM OUT -------------------------------------------------
	//Builds the key path
	key = DM_GUI_FILENAME;
	key = homePath + key;
	//Builds the address
	DM_OutAddress.sun_family = AF_UNIX;
	strcpy(DM_OutAddress.sun_path, key.c_str());

	//HS OUT -------------------------------------------------
	//Builds the key path
	key = HS_GUI_FILENAME;
	key = homePath + key;
	//Builds the address
	HS_OutAddress.sun_family = AF_UNIX;
	strcpy(HS_OutAddress.sun_path, key.c_str());

	//LTM OUT ------------------------------------------------
	//Builds the key path
	key = LTM_GUI_FILENAME;
	key = homePath + key;
	//Builds the address
	LTM_OutAddress.sun_family = AF_UNIX;
	strcpy(LTM_OutAddress.sun_path, key.c_str());

}

void sendToCE()
{
	//Opens the socket
	if ((CE_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(CE_OutSock);
		exit(1);
	}
	//Sends the message
	len = strlen(CE_OutAddress.sun_path) + sizeof(CE_OutAddress.sun_family);
	if (connect(CE_OutSock, (struct sockaddr *)&CE_OutAddress, len) == -1)
	{
		LOG4CXX_ERROR(m_logger,"connect: " << strerror(errno));
		close(CE_OutSock);
		return;
	}

	if (send(CE_OutSock, data, dataLen, 0) == -1)
	{
		LOG4CXX_ERROR(m_logger,"send: " << strerror(errno));
		close(CE_OutSock);
		return;
	}
	close(CE_OutSock);
}

void sendToDM()
{
	//Opens the socket
	if ((DM_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(DM_OutSock);
		exit(1);
	}
	//Sends the message
	len = strlen(DM_OutAddress.sun_path) + sizeof(DM_OutAddress.sun_family);
	if (connect(DM_OutSock, (struct sockaddr *)&DM_OutAddress, len) == -1)
	{
		LOG4CXX_ERROR(m_logger,"connect: " << strerror(errno));
		close(DM_OutSock);
		return;
	}

	if (send(DM_OutSock, data, dataLen, 0) == -1)
	{
		LOG4CXX_ERROR(m_logger,"send: " << strerror(errno));
		close(DM_OutSock);
		return;
	}
	close(DM_OutSock);
}

void sendToHS()
{
	//Opens the socket
	if ((HS_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(HS_OutSock);
		exit(1);
	}
	//Sends the message
	len = strlen(HS_OutAddress.sun_path) + sizeof(HS_OutAddress.sun_family);
	if (connect(HS_OutSock, (struct sockaddr *)&HS_OutAddress, len) == -1)
	{
		LOG4CXX_ERROR(m_logger,"connect: " << strerror(errno));
		close(HS_OutSock);
		return;
	}

	if (send(HS_OutSock, data, dataLen, 0) == -1)
	{
		LOG4CXX_ERROR(m_logger,"send: " << strerror(errno));
		close(HS_OutSock);
		return;
	}
	close(HS_OutSock);
}

void sendToLTM()
{
	//Opens the socket
	if ((LTM_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(LTM_OutSock);
		exit(1);
	}
	//Sends the message
	len = strlen(LTM_OutAddress.sun_path) + sizeof(LTM_OutAddress.sun_family);
	if (connect(LTM_OutSock, (struct sockaddr *)&LTM_OutAddress, len) == -1)
	{
		LOG4CXX_ERROR(m_logger,"connect: " << strerror(errno));
		close(LTM_OutSock);
		return;
	}

	if (send(LTM_OutSock, data, dataLen, 0) == -1)
	{
		LOG4CXX_ERROR(m_logger,"send: " << strerror(errno));
		close(LTM_OutSock);
		return;
	}
	close(LTM_OutSock);
}
void sendAll()
{
	//Opens all the sockets
	//CE OUT -------------------------------------------------
	if ((CE_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(CE_OutSock);
	}

	//DM OUT -------------------------------------------------
	if ((DM_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(DM_OutSock);
	}

	//HS OUT -------------------------------------------------
	if ((HS_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(HS_OutSock);
	}

	//LTM OUT ------------------------------------------------
	if ((LTM_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(LTM_OutSock);
	}


	//Sends the message
	//CE OUT -------------------------------------------------
	len = strlen(CE_OutAddress.sun_path) + sizeof(CE_OutAddress.sun_family);
	if (connect(CE_OutSock, (struct sockaddr *)&CE_OutAddress, len) == -1)
	{
		LOG4CXX_ERROR(m_logger,"connect: " << strerror(errno));
		close(CE_OutSock);
	}

	if (send(CE_OutSock, data, dataLen, 0) == -1)
	{
		LOG4CXX_ERROR(m_logger,"send: " << strerror(errno));
		close(CE_OutSock);
	}
	close(CE_OutSock);
	// -------------------------------------------------------

	//DM OUT -------------------------------------------------
	len = strlen(DM_OutAddress.sun_path) + sizeof(DM_OutAddress.sun_family);
	if (connect(DM_OutSock, (struct sockaddr *)&DM_OutAddress, len) == -1)
	{
		LOG4CXX_ERROR(m_logger,"connect: " << strerror(errno));
		close(DM_OutSock);
	}

	if (send(DM_OutSock, data, dataLen, 0) == -1)
	{
		LOG4CXX_ERROR(m_logger,"send: " << strerror(errno));
		close(DM_OutSock);
	}
	close(DM_OutSock);
	// -------------------------------------------------------


	//HS OUT -------------------------------------------------
	len = strlen(HS_OutAddress.sun_path) + sizeof(HS_OutAddress.sun_family);
	if (connect(HS_OutSock, (struct sockaddr *)&HS_OutAddress, len) == -1)
	{
		LOG4CXX_ERROR(m_logger,"connect: " << strerror(errno));
		close(HS_OutSock);
	}

	if (send(HS_OutSock, data, dataLen, 0) == -1)
	{
		LOG4CXX_ERROR(m_logger,"send: " << strerror(errno));
		close(HS_OutSock);
	}
	close(HS_OutSock);
	// -------------------------------------------------------


	//LTM OUT ------------------------------------------------
	len = strlen(LTM_OutAddress.sun_path) + sizeof(LTM_OutAddress.sun_family);
	if (connect(LTM_OutSock, (struct sockaddr *)&LTM_OutAddress, len) == -1)
	{
		LOG4CXX_ERROR(m_logger,"connect: " << strerror(errno));
		close(LTM_OutSock);
	}

	if (send(LTM_OutSock, data, dataLen, 0) == -1)
	{
		LOG4CXX_ERROR(m_logger,"send: " << strerror(errno));
		close(LTM_OutSock);
	}
	close(LTM_OutSock);
	// -------------------------------------------------------
}

void sclose(int sock)
{
	close(sock);
}

//Returns the number of bits used in the mask when given in in_addr_t form
int getMaskBits(in_addr_t mask)
{
	mask = ~mask;
	int i = 32;
	while(mask != 0)
	{
		mask = mask/2;
		i--;
	}
	return i;
}
