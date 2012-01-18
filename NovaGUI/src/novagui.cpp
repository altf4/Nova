//============================================================================
// Name        : novagui.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : The main NovaGUI component, utilizes the auto-generated ui_novagui.h
//============================================================================/*

#include <sys/socket.h>
#include <QtGui>
#include <QApplication>
#include "novagui.h"
#include "novaconfig.h"
#include "run_popup.h"

#include "NOVAConfiguration.h"
#include "NovaUtil.h"

#include <QString>
#include <QChar>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using namespace std;
using namespace Nova;

//Socket communication variables
int CE_InSock, CE_OutSock, DM_OutSock, HS_OutSock, LTM_OutSock;
struct sockaddr_un CE_InAddress, CE_OutAddress, DM_OutAddress, HS_OutAddress, LTM_OutAddress;
int len;


//GUI to Nova message variables
GUIMsg message = GUIMsg();
u_char msgBuffer[MAX_GUIMSG_SIZE];
int msgLen = 0;

//Receive Suspect variables
u_char buf[MAX_MSG_SIZE];
int bytesRead;

//Configuration variables
bool useTerminals;
char * pathsFile = (char*)"/etc/nova/paths";
string homePath, readPath, writePath;


//General variables like tables, flags, locks, etc.
SuspectHashTable SuspectTable;
pthread_rwlock_t lock;
bool novaRunning = false;


bool featureEnabled[DIM];

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
	SuspectTable.set_empty_key(1);
	subnets.set_empty_key("");
	ports.set_empty_key("");
	nodes.set_empty_key("");
	profiles.set_empty_key("");
	scripts.set_empty_key("");
	subnets.set_deleted_key("Deleted");
	nodes.set_deleted_key("Deleted");
	profiles.set_deleted_key("Deleted");
	ports.set_deleted_key("Deleted");
	scripts.set_deleted_key("Deleted");

	ui.setupUi(this);
	runAsWindowUp = false;
	editingPreferences = false;

	getInfo();

	string novaConfig = "Config/NOVAConfig.txt";
	string logConfig = "Config/Log4cxxConfig_Console.xml";

	openlog("NovaGUI", OPEN_SYSL, LOG_AUTHPRIV);
	prompter = new dialogPrompter();

	loadAll();

	//This register meta type function needs to be called for any object types passed through a signal
	qRegisterMetaType<in_addr_t>("in_addr_t");
	qRegisterMetaType<QItemSelection>("QItemSelection");

	//Sets up the socket addresses
	getSocketAddr();

	//Create listening socket, listen thread and draw thread --------------
	pthread_t CEListenThread;

	if((CE_InSock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		sclose(CE_InSock);
		exit(1);
	}

	len = strlen(CE_InAddress.sun_path) + sizeof(CE_InAddress.sun_family);

	if(bind(CE_InSock,(struct sockaddr *)&CE_InAddress,len) == -1)
	{
		syslog(SYSL_ERR, "Line: %d bind: %s", __LINE__, strerror(errno));
		sclose(CE_InSock);
		exit(1);
	}

	if(listen(CE_InSock, SOCKET_QUEUE_SIZE) == -1)
	{
		syslog(SYSL_ERR, "Line: %d listen: %s", __LINE__, strerror(errno));
		sclose(CE_InSock);
		exit(1);
	}

	//Sets initial view
	this->ui.stackedWidget->setCurrentIndex(0);
	this->ui.mainButton->setFlat(true);
	this->ui.suspectButton->setFlat(false);
	this->ui.doppelButton->setFlat(false);
	this->ui.haystackButton->setFlat(false);

	connect(this, SIGNAL(newSuspect(in_addr_t)), this, SLOT(drawSuspect(in_addr_t)), Qt::BlockingQueuedConnection);

	pthread_create(&CEListenThread,NULL,CEListen, this);
}

NovaGUI::~NovaGUI()
{

}

void NovaGUI::closeEvent(QCloseEvent * e)
{
	e = e;
	closeNova();
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

/************************************************
 * Save Honeyd XML Configuration Functions
 ************************************************/

//Saves the current configuration information to XML files

//**Important** this function assumes that unless it is a new item (ptree pointer == NULL) then
// all required fields exist and old fields have been removed. Ex: if a port previously used a script
// but now has a behavior of open, at that point the user should have erased the script field.
// inverserly if a user switches to script the script field must be created.

//To summarize this function only populates the xml data for the values it contains unless it is a new item,
// it does not clean up, and only creates if it's a new item and then only for the fields that are needed.
// it does not track profile inheritance either, that should be created when the heirarchy is modified.
void NovaGUI::saveAll()
{
	using boost::property_tree::ptree;
	ptree pt;

	//Scripts
	scriptTree.clear();
	for(ScriptTable::iterator it = scripts.begin(); it != scripts.end(); it++)
	{
		pt = it->second.tree;
		pt.put<std::string>("name", it->second.name);
		pt.put<std::string>("path", it->second.path);
		scriptTree.add_child("scripts.script", pt);
	}

	//Ports
	portTree.clear();
	for(PortTable::iterator it = ports.begin(); it != ports.end(); it++)
	{
		pt = it->second.tree;
		pt.put<std::string>("name", it->second.portName);
		pt.put<std::string>("number", it->second.portNum);
		pt.put<std::string>("type", it->second.type);
		pt.put<std::string>("behavior", it->second.behavior);
		//If this port uses a script, save it.
		if(!it->second.behavior.compare("script") || !it->second.behavior.compare("internal"))
		{
			pt.put<std::string>("script", it->second.scriptName);
		}
		//If the port works as a proxy, save destination
		else if(!it->second.behavior.compare("proxy"))
		{
			pt.put<std::string>("IP", it->second.proxyIP);
			pt.put<std::string>("Port", it->second.proxyPort);
		}
		portTree.add_child("ports.port", pt);
	}

	subnetTree->clear();
	for(SubnetTable::iterator it = subnets.begin(); it != subnets.end(); it++)
	{
		pt = it->second.tree;

		//TODO assumes subnet is interface, need to discover and handle if virtual
		pt.put<std::string>("name", it->second.name);
		pt.put<bool>("enabled",it->second.enabled);

		//Remove /## format mask from the address then put it in the XML.
		stringstream ss;
		ss << "/" << it->second.maskBits;
		int i = ss.str().size();
		string temp = it->second.address.substr(0,(it->second.address.size()-i));
		pt.put<std::string>("IP", temp);

		//Gets the mask from mask bits then put it in XML
		in_addr_t mask = pow(2, 32-it->second.maskBits) - 1;
		//If maskBits is 24 then we have 2^8 -1 = 0x000000FF
		mask = ~mask; //After getting the inverse of this we have the mask in host addr form.
		//Convert to network order, put in in_addr struct
		struct in_addr addr;
		addr.s_addr = htonl(mask);
		//call ntoa to get char * and make string
		temp = string(inet_ntoa(addr));
		pt.put<std::string>("mask", temp);
		subnetTree->add_child("interface", pt);
	}

	//Nodes
	nodesTree->clear();
	for(NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
	{
		pt = it->second.tree;
		//Required xml entires
		pt.put<std::string>("interface", it->second.interface);
		pt.put<std::string>("IP", it->second.address);
		pt.put<bool>("enabled", it->second.enabled);
		pt.put<std::string>("profile.name", it->second.pfile);
		nodesTree->add_child("node",pt);
	}
	profileTree.clear();
	for(ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
	{
		if(it->second.parentProfile == "")
		{
			pt = it->second.tree;
			profileTree.add_child("profiles.profile", pt);
		}
	}
	write_xml(homePath+"/scripts.xml", scriptTree);
	write_xml(homePath+"/templates/ports.xml", portTree);
	write_xml(homePath+"/templates/nodes.xml", groupTree);
	write_xml(homePath+"/templates/profiles.xml", profileTree);
}

//Writes the current configuration to honeyd configs
//TODO need to take current XML files and write them as honeyd configuration, called in startNova()
void NovaGUI::writeHoneyd()
{

}

/************************************************
 * Load Honeyd XML Configuration Functions
 ************************************************/

//Calls all load functions
void NovaGUI::loadAll()
{
	scripts.clear_no_resize();
	ports.clear_no_resize();
	profiles.clear_no_resize();
	nodes.clear_no_resize();
	subnets.clear_no_resize();

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
			s.tree = v.second;
			//Each script consists of a name and path to that script
			s.name = v.second.get<std::string>("name");
			s.path = v.second.get<std::string>("path");
			scripts[s.name] = s;
		}
	}
	catch(std::exception &e)
	{
		syslog(SYSL_ERR, "Line: %d Problem loading scripts: %s", __LINE__, string(e.what()).c_str());
		prompter->displayPrompt(HONEYD_FILE_READ_FAIL, string(e.what()).c_str());
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
			p.tree = v.second;
			//Required xml entries
			p.portName = v.second.get<std::string>("name");
			p.portNum = v.second.get<std::string>("number");
			p.type = v.second.get<std::string>("type");
			p.behavior = v.second.get<std::string>("behavior");

			//If this port uses a script, find and assign it.
			if(!p.behavior.compare("script") || !p.behavior.compare("internal"))
			{
				p.scriptName = v.second.get<std::string>("script");
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
		syslog(SYSL_ERR, "Line: %d Problem loading ports: %s", __LINE__, string(e.what()).c_str());
		prompter->displayPrompt(HONEYD_FILE_READ_FAIL, string(e.what()).c_str());
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
						syslog(SYSL_ERR, "Line: %d Problem loading nodes: %s", __LINE__, string(e.what()).c_str());
						prompter->displayPrompt(HONEYD_FILE_READ_FAIL, string(e.what()).c_str());
					}
				}
				catch(std::exception &e)
				{
					syslog(SYSL_ERR, "Line: %d Problem loading subnets: %s", __LINE__, string(e.what()).c_str());
					prompter->displayPrompt(HONEYD_FILE_READ_FAIL, string(e.what()).c_str());
				}
			}
		}
	}
	catch(std::exception &e)
	{
		syslog(SYSL_ERR, "Line: %d Problem loading group: %s - %s", __LINE__, group.c_str(), string(e.what()).c_str());
		prompter->displayPrompt(HONEYD_FILE_READ_FAIL, string(e.what()).c_str());
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
				sub.tree = v.second;
				//Extract the data
				sub.name = v.second.get<std::string>("name");
				sub.address = v.second.get<std::string>("IP");
				sub.mask = v.second.get<std::string>("mask");
				sub.enabled = v.second.get<bool>("enabled");

				//Gets the IP address in uint32 form
				in_addr_t baseTemp = ntohl(inet_addr(sub.address.c_str()));

				//Converting the mask to uint32 allows a simple bitwise AND to get the lowest IP in the subnet.
				in_addr_t maskTemp = ntohl(inet_addr(sub.mask.c_str()));
				sub.base = (baseTemp & maskTemp);
				//Get the number of bits in the mask
				sub.maskBits = GetMaskBits(maskTemp);
				//Adding the binary inversion of the mask gets the highest usable IP
				sub.max = sub.base + ~maskTemp;
				stringstream ss;
				ss << sub.address << "/" << sub.maskBits;
				sub.address = ss.str();

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
				syslog(SYSL_ERR, "Line: %d Unexpected Entry in file: %s", __LINE__, string(v.first.data()).c_str());
				prompter->displayPrompt(UNEXPECTED_FILE_ENTRY, string(v.first.data()).c_str());
			}
		}
	}
	catch(std::exception &e)
	{
		syslog(SYSL_ERR, "Line: %d Problem loading subnets: %s", __LINE__, string(e.what()).c_str());
		prompter->displayPrompt(HONEYD_LOAD_SUBNETS_FAIL, string(e.what()).c_str());
	}
}


//loads haystack nodes from file for current group
void NovaGUI::loadNodes(ptree *ptr)
{
	profile p;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			if(!string(v.first.data()).compare("node"))
			{
				node n;
				n.tree = v.second;
				//Required xml entires
				n.interface = v.second.get<std::string>("interface");
				n.address = v.second.get<std::string>("IP");
				n.enabled = v.second.get<bool>("enabled");
				n.pfile = v.second.get<std::string>("profile.name");

				//intialize subnet to NULL and check for smallest bounding subnet
				n.sub = "";

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
							n.sub = it->second.address;
						}
					}
				}

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
				if((n.sub != "") && unique)
				{
					//save the node in the table
					nodes[n.address] = n;

					//Put address of saved node in subnet's list of nodes.
					subnets[nodes[n.address].sub].nodes.push_back(n.address);
				}
				//If no subnet found, can't use node unless it's doppelganger.
				else
				{
					syslog(SYSL_ERR, "Line: %d Node at IP: %s is outside all valid subnet ranges", __LINE__, n.address.c_str());
					prompter->displayPrompt(HONEYD_NODE_INVALID_SUBNET, n.address);
				}
			}
			else
			{
				syslog(SYSL_ERR, "Line: %d Unexpected Entry in file: %s", __LINE__, string(v.first.data()).c_str());
				prompter->displayPrompt(UNEXPECTED_FILE_ENTRY, string(v.first.data()).c_str());
			}
		}
	}
	catch(std::exception &e)
	{
		syslog(SYSL_ERR, "Line: %d Problem loading nodes: %s", __LINE__, string(e.what()).c_str());
		prompter->displayPrompt(HONEYD_LOAD_NODES_FAIL, string(e.what()).c_str());
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
				p.parentProfile = "";
				p.tree = v.second;

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
					//start recurisive descent down profile tree with this profile as the root
					//pass subtree and pointer to parent
					loadSubProfiles(p.name);
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
				syslog(SYSL_ERR, "Line: %d Invalid XML Path %s", __LINE__, string(v.first.data()).c_str());
			}
		}
	}
	catch(std::exception &e)
	{
		syslog(SYSL_ERR, "Line: %d Problem loading Profiles: %s", __LINE__, string(e.what()).c_str());
		prompter->displayPrompt(HONEYD_LOAD_PROFILES_FAIL, string(e.what()).c_str());
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
		syslog(SYSL_ERR, "Line: %d Problem loading profile set parameters: %s", __LINE__, string(e.what()).c_str());
		prompter->displayPrompt(HONEYD_LOAD_PROFILESET_FAIL, string(e.what()).c_str());
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
						if(!prt->portNum.compare(ports[p->ports[i]].portNum) && !prt->type.compare(ports[p->ports[i]].type))
						{
							p->ports.erase(p->ports.begin()+i);
						}
					}
					//Add specified port
					p->ports.push_back(prt->portName);
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
		syslog(SYSL_ERR, "Line: %d Problem loading profile add parameters: %s", __LINE__, string(e.what()).c_str());
		prompter->displayPrompt(HONEYD_LOAD_PROFILES_FAIL, string(e.what()).c_str());
	}
}

//Recursive descent down a profile tree, inherits parent, sets values and continues if not leaf.
void NovaGUI::loadSubProfiles(string parent)
{
	ptree ptr = profiles[parent].tree;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr.get_child("profiles"))
		{
			ptree *ptr2;

			//Inherits parent,
			profile prof = profiles[parent];
			prof.tree = v.second;
			prof.parentProfile = parent;

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
				loadSubProfiles(prof.name);
			}
			catch(...){}
		}
	}
	catch(std::exception &e)
	{
		syslog(SYSL_ERR, "Line: %d Problem loading sub profiles: %s", __LINE__, string(e.what()).c_str());
		prompter->displayPrompt(HONEYD_LOAD_PROFILES_FAIL, string(e.what()).c_str());
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
		syslog(SYSL_ERR, "Line: %d accept: %s", __LINE__, strerror(errno));
		sclose(connectionSocket);
		return false;
	}
	if((bytesRead = recv(connectionSocket, buf, MAX_MSG_SIZE, 0 )) == 0)
	{
		return false;
	}
	else if(bytesRead == -1)
	{
		syslog(SYSL_ERR, "Line: %d recv: %s", __LINE__, strerror(errno));
		sclose(connectionSocket);
		return false;
	}
	sclose(connectionSocket);

	Suspect* suspect = new Suspect();

	try
	{
		suspect->DeserializeSuspect(buf);
		bzero(buf, bytesRead);
	}
	catch(std::exception e)
	{
		syslog(SYSL_ERR, "Line: %d Error interpreting received Suspect: %s", __LINE__, string(e.what()).c_str());
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
	//If the suspect already exists in our table
	if(SuspectTable.find(suspectItem.suspect->IP_address.s_addr) != SuspectTable.end())
	{
		//We point to the same item so it doesn't need to be deleted.
		suspectItem.item = SuspectTable[suspectItem.suspect->IP_address.s_addr].item;
		suspectItem.mainItem = SuspectTable[suspectItem.suspect->IP_address.s_addr].mainItem;

		//Delete the old Suspect since we created and pointed to a new one
		delete SuspectTable[suspectItem.suspect->IP_address.s_addr].suspect;
	}
	//We borrow the flag to show there is new information.
	suspectItem.suspect->needs_feature_update = true;
	//Update the entry in the table
	SuspectTable[suspectItem.suspect->IP_address.s_addr] = suspectItem;
	pthread_rwlock_unlock(&lock);
	emit newSuspect(suspectItem.suspect->IP_address.s_addr);
}

/************************************************
 * Display Functions
 ************************************************/

void NovaGUI::drawAllSuspects()
{
	clearSuspectList();

	QListWidgetItem * item = NULL;
	QListWidgetItem * mainItem = NULL;
	Suspect * suspect = NULL;
	QString str;
	QBrush brush;
	QColor color;

	pthread_rwlock_wrlock(&lock);
	for (SuspectHashTable::iterator it = SuspectTable.begin() ; it != SuspectTable.end(); it++)
	{
		str = (QString) string(inet_ntoa(it->second.suspect->IP_address)).c_str();
		suspect = it->second.suspect;
		//Create the colors for the draw
		if(suspect->classification < 0.5)
		{
			//at 0.5 QBrush is 255,255 (yellow), from 0->0.5 include more red until yellow
			color = QColor((int)(200*2*suspect->classification),200, 50);
		}
		else
		{
			//at 0.5 QBrush is 255,255 (yellow), at from 0.5->1.0 remove more green until QBrush is Red
			color = QColor(200,200-(int)(200*2*(suspect->classification-0.5)), 50);
		}
		brush.setColor(color);
		brush.setStyle(Qt::NoBrush);

		//Create the Suspect in list with info set alignment and color
		item = new QListWidgetItem(str,0);
		item->setTextAlignment(Qt::AlignLeft|Qt::AlignBottom);
		item->setForeground(brush);

		in_addr_t addr;
		int i = 0;
		if(ui.suspectList->count())
		{
			for(i = 0; i < ui.suspectList->count(); i++)
			{
				addr = inet_addr(ui.suspectList->item(i)->text().toStdString().c_str());
				if(SuspectTable[addr].suspect->classification < suspect->classification)
					break;
			}
		}
		ui.suspectList->insertItem(i, item);

		//If Hostile
		if(suspect->isHostile )
		{
			//Copy the item and add it to the list
			mainItem = new QListWidgetItem(str,0);
			mainItem->setTextAlignment(Qt::AlignLeft|Qt::AlignBottom);
			mainItem->setForeground(brush);

			i = 0;
			if(ui.hostileList->count())
			{
				for(i = 0; i < ui.hostileList->count(); i++)
				{
					addr = inet_addr(ui.hostileList->item(i)->text().toStdString().c_str());
					if(SuspectTable[addr].suspect->classification < suspect->classification)
						break;
				}
			}
			ui.hostileList->insertItem(i, mainItem);
			it->second.mainItem = mainItem;
		}
		//Point to the new items
		it->second.item = item;
		//Reset the flags
		suspect->needs_feature_update = false;
		it->second.suspect = suspect;
	}
	updateSuspectWidgets();
	pthread_rwlock_unlock(&lock);
}

//Updates the UI with the latest suspect information
//*NOTE This slot is not thread safe, make sure you set appropriate locks before sending a signal to this slot
void NovaGUI::drawSuspect(in_addr_t suspectAddr)
{

	QString str;
	QBrush brush;
	QColor color;
	in_addr_t addr;

	pthread_rwlock_wrlock(&lock);
	suspectItem * sItem = &SuspectTable[suspectAddr];
	//Extract Information
	str = (QString) string(inet_ntoa(sItem->suspect->IP_address)).c_str();

	//Create the colors for the draw
	if(sItem->suspect->classification < 0.5)
	{
		//at 0.5 QBrush is 255,255 (yellow), from 0->0.5 include more red until yellow
		color = QColor((int)(200*2*sItem->suspect->classification),200, 50);
	}
	else
	{
		//at 0.5 QBrush is 255,255 (yellow), at from 0.5->1.0 remove more green until QBrush is Red
		color = QColor(200,200-(int)(200*2*(sItem->suspect->classification-0.5)), 50);
	}
	brush.setColor(color);
	brush.setStyle(Qt::NoBrush);

	//If the item exists
	if(sItem->item != NULL)
	{
		sItem->item->setText(str);
		sItem->item->setForeground(brush);
		bool selected = false;
		int current_row = ui.suspectList->currentRow();

		//If this is our current selection flag it so we can update the selection if we change the index
		if(current_row == ui.suspectList->row(sItem->item))
			selected = true;

		ui.suspectList->removeItemWidget(sItem->item);

		int i = 0;
		if(ui.suspectList->count())
		{
			for(i = 0; i < ui.suspectList->count(); i++)
			{
				addr = inet_addr(ui.suspectList->item(i)->text().toStdString().c_str());
				if(SuspectTable[addr].suspect->classification < sItem->suspect->classification)
					break;
			}
		}
		ui.suspectList->insertItem(i, sItem->item);

		//If we need to update the selection
		if(selected)
		{
			ui.suspectList->setCurrentRow(i);
		}
	}
	//If the item doesn't exist
	else
	{
		//Create the Suspect in list with info set alignment and color
		sItem->item = new QListWidgetItem(str,0);
		sItem->item->setTextAlignment(Qt::AlignLeft|Qt::AlignBottom);
		sItem->item->setForeground(brush);

		int i = 0;
		if(ui.suspectList->count())
		{
			for(i = 0; i < ui.suspectList->count(); i++)
			{
				addr = inet_addr(ui.suspectList->item(i)->text().toStdString().c_str());
				if(SuspectTable[addr].suspect->classification < sItem->suspect->classification)
					break;
			}
		}
		ui.suspectList->insertItem(i, sItem->item);
	}

	//If the mainItem exists and suspect is hostile
	if((sItem->mainItem != NULL) && sItem->suspect->isHostile)
	{
		sItem->mainItem->setText(str);
		sItem->mainItem->setForeground(brush);
		bool selected = false;
		int current_row = ui.hostileList->currentRow();

		//If this is our current selection flag it so we can update the selection if we change the index
		if(current_row == ui.hostileList->row(sItem->mainItem))
			selected = true;

		ui.hostileList->removeItemWidget(sItem->mainItem);
		int i = 0;
		if(ui.hostileList->count())
		{
			for(i = 0; i < ui.hostileList->count(); i++)
			{
				addr = inet_addr(ui.hostileList->item(i)->text().toStdString().c_str());
				if(SuspectTable[addr].suspect->classification < sItem->suspect->classification)
					break;
			}
		}
		ui.hostileList->insertItem(i, sItem->mainItem);

		//If we need to update the selection
		if(selected)
		{
			ui.hostileList->setCurrentRow(i);
		}
		sItem->mainItem->setToolTip(QString(sItem->suspect->ToString(featureEnabled).c_str()));
	}
	//Else if the mainItem exists and suspect is not hostile
	else if(sItem->mainItem != NULL)
	{
		ui.hostileList->removeItemWidget(sItem->mainItem);
	}
	//If the mainItem doesn't exist and suspect is hostile
	else if(sItem->suspect->isHostile)
	{
		//Create the Suspect in list with info set alignment and color
		sItem->mainItem = new QListWidgetItem(str,0);
		sItem->mainItem->setTextAlignment(Qt::AlignLeft|Qt::AlignBottom);
		sItem->mainItem->setForeground(brush);

		sItem->mainItem->setToolTip(QString(sItem->suspect->ToString(featureEnabled).c_str()));

		int i = 0;
		if(ui.hostileList->count())
		{
			for(i = 0; i < ui.hostileList->count(); i++)
			{
				addr = inet_addr(ui.hostileList->item(i)->text().toStdString().c_str());
				if(SuspectTable[addr].suspect->classification < sItem->suspect->classification)
					break;
			}
		}
		ui.hostileList->insertItem(i, sItem->mainItem);
	}
	sItem->item->setToolTip(QString(sItem->suspect->ToString(featureEnabled).c_str()));
	updateSuspectWidgets();
	pthread_rwlock_unlock(&lock);
}
void NovaGUI::updateSuspectWidgets()
{
	double hostileAcc = 0, benignAcc = 0, totalAcc = 0;

	for (SuspectHashTable::iterator it = SuspectTable.begin() ; it != SuspectTable.end(); it++)
	{
		if(it->second.suspect->isHostile)
		{
			hostileAcc += it->second.suspect->classification;
			totalAcc += it->second.suspect->classification;
		}
		else
		{
			benignAcc += 1-it->second.suspect->classification;
			totalAcc += 1-it->second.suspect->classification;
		}
	}

	int numBenign = ui.suspectList->count() - ui.hostileList->count();
	stringstream ss;
	ss << numBenign;
	ui.numBenignEdit->setText(QString(ss.str().c_str()));

	if(numBenign)
	{
		benignAcc /= numBenign;
		ui.benignClassificationBar->setValue((int)(benignAcc*100));
		ui.benignSuspectClassificationBar->setValue((int)(benignAcc*100));
	}
	else
	{
		ui.benignClassificationBar->setValue(100);
		ui.benignSuspectClassificationBar->setValue(100);
	}
	if(ui.hostileList->count())
	{
		hostileAcc /= ui.hostileList->count();
		ui.hostileClassificationBar->setValue((int)(hostileAcc*100));
		ui.hostileSuspectClassificationBar->setValue((int)(hostileAcc*100));
	}
	else
	{
		ui.hostileClassificationBar->setValue(100);
		ui.hostileSuspectClassificationBar->setValue(100);
	}
	if(ui.suspectList->count())
	{
		totalAcc /= ui.suspectList->count();
		ui.overallSuspectClassificationBar->setValue((int)(totalAcc*100));
	}
	else
	{
		ui.overallSuspectClassificationBar->setValue(100);
	}
}
void NovaGUI::saveSuspects()
{
	 QString filename = QFileDialog::getSaveFileName(this,
			tr("Save Suspect List"), QDir::currentPath(),
			tr("Documents (*.txt)"));

	if (filename.isNull())
	{
		return;
	}


	message.SetMessage(WRITE_SUSPECTS, filename.toStdString());
	msgLen = message.SerialzeMessage(msgBuffer);

	//Sends the message to all Nova processes
	sendToCE();
}

void NovaGUI::clearSuspectList()
{
	pthread_rwlock_wrlock(&lock);
	this->ui.suspectList->clear();
	this->ui.hostileList->clear();
	//Since clearing permenantly deletes the items we need to make sure the suspects point to null
	for (SuspectHashTable::iterator it = SuspectTable.begin() ; it != SuspectTable.end(); it++)
	{
		it->second.item = NULL;
		it->second.mainItem = NULL;
	}
	pthread_rwlock_unlock(&lock);
}

void NovaGUI::drawNodes()
{
	//QTreeWidgetItem * item = NULL;
	QString str;

	ui.nodesTreeWidget->clear();

	for(SubnetTable::iterator it = subnets.begin(); it != subnets.end(); it++)
	{
		//item = new QTreeWidgetItem(ui.nodesTreeWidget);
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

void NovaGUI::on_actionSave_Suspects_triggered()
{
	saveSuspects();
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

void NovaGUI::on_runButton_clicked()
{
	startNova();
}
void NovaGUI::on_stopButton_clicked()
{
	closeNova();
}
void NovaGUI::on_clearSuspectsButton_clicked()
{
	drawAllSuspects();
	clearSuspects();
}


/************************************************
 * List Signal Handlers
 ************************************************/
void NovaGUI::on_suspectList_itemSelectionChanged()
{
	pthread_rwlock_rdlock(&lock);
	in_addr_t addr = inet_addr(ui.suspectList->currentItem()->text().toStdString().c_str());
	ui.suspectFeaturesEdit->setText(QString(SuspectTable[addr].suspect->ToString(featureEnabled).c_str()));
	pthread_rwlock_unlock(&lock);
}
/************************************************
 * IPC Functions
 ************************************************/

void clearSuspects()
{
	pthread_rwlock_wrlock(&lock);
	SuspectTable.clear();
	message.SetMessage(CLEAR_ALL);
	msgLen = message.SerialzeMessage(msgBuffer);
	sendToCE();
	sendToDM();
	pthread_rwlock_unlock(&lock);
}

void closeNova()
{
	if(novaRunning)
	{
		//Sets the message
		message.SetMessage(EXIT);
		msgLen = message.SerialzeMessage(msgBuffer);

		//Sends the message to all Nova processes
		sendAll();

		// Close Honeyd processes
		FILE * out = popen("pidof honeyd","r");
		if(out != NULL)
		{
			char buffer[1024];
			char * line = fgets(buffer, sizeof(buffer), out);
			string cmd = "sudo kill " + string(line);
			if(cmd.size() > 5)
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
		string homePath = GetHomePath();
		string input = homePath + "/Config/NOVAConfig.txt";

		NOVAConfiguration * NovaConfig = new NOVAConfiguration();
		NovaConfig->LoadConfig((char*)input.c_str(), homePath);

		if (!NovaConfig->options["USE_TERMINALS"].isValid || !NovaConfig->options["ENABLED_FEATURES"].isValid)
		{
			syslog(SYSL_ERR, "Line: %d ERROR: Unable to load configuration file.", __LINE__);
		}

		useTerminals = atoi(NovaConfig->options["USE_TERMINALS"].data.c_str());
		string enabledFeatureMask = NovaConfig->options["ENABLED_FEATURES"].data;

		for (uint i = 0; i < DIM; i++)
		{
			if ('1' == enabledFeatureMask.at(i))
			{
				featureEnabled[i] = true;
			}
			else
			{
				featureEnabled[i] = false;
			}
		}

		if(!useTerminals)
		{
			system(("nohup sudo honeyd -i eth0 -f "+homePath+"/Config/haystack.config -p "+readPath+"/nmap-os-db"
					" -s "+writePath+"/Logs/honeydservice.log > /dev/null &").c_str());
			system(("nohup sudo honeyd -i lo -f "+homePath+"/Config/doppelganger.config -p "+readPath+"/nmap-os-db"
					" -s "+writePath+"/Logs/honeydDoppservice.log 10.0.0.0/8 > /dev/null &").c_str());
			system("nohup LocalTrafficMonitor > /dev/null &");
			system("nohup Haystack > /dev/null &");
			system("nohup ClassificationEngine > /dev/null &");
			system("nohup DoppelgangerModule > /dev/null &");
		}
		else
		{
			system(("(gnome-terminal -t \"HoneyD Haystack\" --geometry \"+0+0\" -x sudo honeyd -d -i eth0 -f "+homePath+"/Config/haystack.config"
					" -p "+readPath+"/nmap-os-db -s "+writePath+"/Logs/honeydservice.log )&").c_str());
			system(("(gnome-terminal -t \"HoneyD Doppelganger\" --geometry \"+500+0\" -x sudo honeyd -d -i lo -f "+homePath+"/Config/doppelganger.config"
					" -p "+readPath+"/nmap-os-db -s "+writePath+"/Logs/honeydDoppservice.log 10.0.0.0/8 )&").c_str());
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
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(CE_OutSock);
		exit(1);
	}
	//Sends the message
	len = strlen(CE_OutAddress.sun_path) + sizeof(CE_OutAddress.sun_family);
	if (connect(CE_OutSock, (struct sockaddr *)&CE_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "Line: %d connect: %s", __LINE__, strerror(errno));
		close(CE_OutSock);
		return;
	}

	if (send(CE_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "Line: %d send: %s", __LINE__, strerror(errno));
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
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(DM_OutSock);
		exit(1);
	}
	//Sends the message
	len = strlen(DM_OutAddress.sun_path) + sizeof(DM_OutAddress.sun_family);
	if (connect(DM_OutSock, (struct sockaddr *)&DM_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "Line: %d connect: %s", __LINE__, strerror(errno));
		close(DM_OutSock);
		return;
	}

	if (send(DM_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "Line: %d send: %s", __LINE__, strerror(errno));
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
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(HS_OutSock);
		exit(1);
	}
	//Sends the message
	len = strlen(HS_OutAddress.sun_path) + sizeof(HS_OutAddress.sun_family);
	if (connect(HS_OutSock, (struct sockaddr *)&HS_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "Line: %d connect: %s", __LINE__, strerror(errno));
		close(HS_OutSock);
		return;
	}

	if (send(HS_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "Line: %d send: %s", __LINE__, strerror(errno));
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
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(LTM_OutSock);
		exit(1);
	}
	//Sends the message
	len = strlen(LTM_OutAddress.sun_path) + sizeof(LTM_OutAddress.sun_family);
	if (connect(LTM_OutSock, (struct sockaddr *)&LTM_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "Line: %d connect: %s", __LINE__, strerror(errno));
		close(LTM_OutSock);
		return;
	}

	if (send(LTM_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "Line: %d send: %s", __LINE__, strerror(errno));
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
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(CE_OutSock);
	}

	//DM OUT -------------------------------------------------
	if ((DM_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(DM_OutSock);
	}

	//HS OUT -------------------------------------------------
	if ((HS_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(HS_OutSock);
	}

	//LTM OUT ------------------------------------------------
	if ((LTM_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		syslog(SYSL_ERR, "Line: %d socket: %s", __LINE__, strerror(errno));
		close(LTM_OutSock);
	}


	//Sends the message
	//CE OUT -------------------------------------------------
	len = strlen(CE_OutAddress.sun_path) + sizeof(CE_OutAddress.sun_family);
	if (connect(CE_OutSock, (struct sockaddr *)&CE_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "Line: %d connect: %s", __LINE__, strerror(errno));
		close(CE_OutSock);
	}

	if (send(CE_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "Line: %d send: %s", __LINE__, strerror(errno));
		close(CE_OutSock);
	}
	close(CE_OutSock);
	// -------------------------------------------------------

	//DM OUT -------------------------------------------------
	len = strlen(DM_OutAddress.sun_path) + sizeof(DM_OutAddress.sun_family);
	if (connect(DM_OutSock, (struct sockaddr *)&DM_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "Line: %d connect: %s", __LINE__, strerror(errno));
		close(DM_OutSock);
	}

	if (send(DM_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "Line: %d send: %s", __LINE__, strerror(errno));
		close(DM_OutSock);
	}
	close(DM_OutSock);
	// -------------------------------------------------------


	//HS OUT -------------------------------------------------
	len = strlen(HS_OutAddress.sun_path) + sizeof(HS_OutAddress.sun_family);
	if (connect(HS_OutSock, (struct sockaddr *)&HS_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "Line: %d connect: %s", __LINE__, strerror(errno));
		close(HS_OutSock);
	}

	if (send(HS_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "Line: %d send: %s", __LINE__, strerror(errno));
		close(HS_OutSock);
	}
	close(HS_OutSock);
	// -------------------------------------------------------


	//LTM OUT ------------------------------------------------
	len = strlen(LTM_OutAddress.sun_path) + sizeof(LTM_OutAddress.sun_family);
	if (connect(LTM_OutSock, (struct sockaddr *)&LTM_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "Line: %d connect: %s", __LINE__, strerror(errno));
		close(LTM_OutSock);
	}

	if (send(LTM_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "Line: %d send: %s", __LINE__, strerror(errno));
		close(LTM_OutSock);
	}
	close(LTM_OutSock);
	// -------------------------------------------------------
}

void sclose(int sock)
{
	close(sock);
}
