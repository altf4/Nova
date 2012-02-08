//============================================================================
// Name        : novagui.cpp
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
// Description : The main NovaGUI component, utilizes the auto-generated ui_novagui.h
//============================================================================
#include "novagui.h"
#include "classifierPrompt.h"


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
bool useTerminals = true;
char * pathsFile = (char*)"/etc/nova/paths";
string homePath, readPath, writePath;
string configurationFile = "/Config/NOVAConfig.txt";
NOVAConfiguration configuration;

// Paths extracted from the config file
string doppelgangerPath;
string haystackPath;
string trainingDataPath;
bool isTraining = false;

//General variables like tables, flags, locks, etc.
SuspectHashTable SuspectTable;
pthread_rwlock_t lock;

bool featureEnabled[DIM];
bool editingSuspectList = false;
bool isHelpUp = false;
QMenu * suspectMenu;

// Defines the order of components in the process list and novaComponents array
#define COMPONENT_CE 0
#define COMPONENT_LM 1
#define COMPONENT_DM 2
#define COMPONENT_DMH 3
#define COMPONENT_HS 4
#define COMPONENT_HSH 5


struct novaComponent novaComponents[6];

/************************************************
 * Constructors, Destructors and Closing Actions
 ************************************************/

//Called when process receives a SIGINT, like if you press ctrl+c
void sighandler(int param)
{
	param = param;
	stopNova();
	exit(1);
}

NovaGUI::NovaGUI(QWidget *parent)
    : QMainWindow(parent)
{
	signal(SIGINT, sighandler);
	pthread_rwlock_init(&lock, NULL);
	SuspectTable.set_empty_key(1);
	SuspectTable.set_deleted_key(5);
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

	//Pre-forms the suspect menu
	suspectMenu = new QMenu(this);

	runAsWindowUp = false;
	editingPreferences = false;
	isHelpUp = false;

	openlog("NovaGUI", OPEN_SYSL, LOG_AUTHPRIV);

	if( !NOVAConfiguration::InitUserConfigs(GetHomePath()) )
	{
		exit(EXIT_FAILURE);
	}

	getInfo();
	initiateSystemStatus();

	string novaConfig = "Config/NOVAConfig.txt";

	// Create the dialog generator
	prompter= new DialogPrompter();

	// Register our desired error message types
	messageType * t = new messageType();
	t->descriptionUID = "Failure reading config files";
	t->action = CHOICE_SHOW;
	t->type = errorPrompt;
	CONFIG_READ_FAIL = prompter->RegisterDialog(*t);

	t->descriptionUID = "Failure writing config files";
	t->action = CHOICE_SHOW;
	t->type = errorPrompt;
	CONFIG_WRITE_FAIL = prompter->RegisterDialog(*t);

	t->descriptionUID = "Failure reading honeyd files";
	t->action = CHOICE_SHOW;
	t->type = errorPrompt;
	HONEYD_READ_FAIL = prompter->RegisterDialog(*t);

	t->descriptionUID = "Failure loading honeyd config files";
	t->action = CHOICE_SHOW;
	t->type = errorPrompt;
	HONEYD_LOAD_FAIL = prompter->RegisterDialog(*t);

	t->descriptionUID = "Unexpected file entries";
	t->action = CHOICE_SHOW;
	t->type = errorPrompt;
	UNEXPECTED_ENTRY = prompter->RegisterDialog(*t);

	t->descriptionUID = "Honeyd subnets out of range";
	t->action = CHOICE_SHOW;
	t->type = errorPrompt;
	HONEYD_INVALID_SUBNET = prompter->RegisterDialog(*t);

	t->descriptionUID = "Request to merge CE capture into training Db";
	t->action = CHOICE_SHOW;
	t->type = notifyActionPrompt;
	LAUNCH_TRAINING_MERGE = prompter->RegisterDialog(*t);


	loadAll();

	//This register meta type function needs to be called for any object types passed through a signal
	qRegisterMetaType<in_addr_t>("in_addr_t");
	qRegisterMetaType<QItemSelection>("QItemSelection");

	//Sets up the socket addresses
	getSocketAddr();

	//Create listening socket, listen thread and draw thread --------------
	pthread_t CEListenThread;
	pthread_t StatusUpdateThread;

	if((CE_InSock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d socket: %s", __FILE__, __LINE__, strerror(errno));
		sclose(CE_InSock);
		exit(1);
	}

	len = strlen(CE_InAddress.sun_path) + sizeof(CE_InAddress.sun_family);

	if(bind(CE_InSock,(struct sockaddr *)&CE_InAddress,len) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d bind: %s", __FILE__, __LINE__, strerror(errno));
		sclose(CE_InSock);
		exit(1);
	}

	if(listen(CE_InSock, SOCKET_QUEUE_SIZE) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d listen: %s", __FILE__, __LINE__, strerror(errno));
		sclose(CE_InSock);
		exit(1);
	}


	configurationFile = homePath + configurationFile;
	configuration.LoadConfig(configurationFile.c_str(), homePath, __FILE__);

	doppelgangerPath = configuration.options["DM_HONEYD_CONFIG"].data;
	haystackPath = configuration.options["HS_HONEYD_CONFIG"].data;
	trainingDataPath = configuration.options["DATAFILE"].data;
	isTraining = atoi(configuration.options["IS_TRAINING"].data.c_str());



	//Sets initial view
	this->ui.stackedWidget->setCurrentIndex(0);
	this->ui.mainButton->setFlat(true);
	this->ui.suspectButton->setFlat(false);
	this->ui.doppelButton->setFlat(false);
	this->ui.haystackButton->setFlat(false);
	connect(this, SIGNAL(newSuspect(in_addr_t)), this, SLOT(drawSuspect(in_addr_t)), Qt::BlockingQueuedConnection);
	connect(this, SIGNAL(refreshSystemStatus()), this, SLOT(updateSystemStatus()), Qt::BlockingQueuedConnection);

	pthread_create(&CEListenThread,NULL,CEListen, this);
	pthread_create(&StatusUpdateThread,NULL,StatusUpdate, this);
}

NovaGUI::~NovaGUI()
{

}


//Draws the suspect context menu
void NovaGUI::contextMenuEvent(QContextMenuEvent * event)
{
	if(ui.suspectList->hasFocus() || ui.suspectList->underMouse())
	{
		suspectMenu->clear();
		if(ui.suspectList->isItemSelected(ui.suspectList->currentItem()))
		{
			suspectMenu->addAction(ui.actionClear_Suspect);
			suspectMenu->addAction(ui.actionHide_Suspect);
		}
	}
	else if(ui.hostileList->hasFocus() || ui.hostileList->underMouse())
	{
		suspectMenu->clear();
		if(ui.hostileList->isItemSelected(ui.hostileList->currentItem()))
		{
			suspectMenu->addAction(ui.actionClear_Suspect);
			suspectMenu->addAction(ui.actionHide_Suspect);
		}
	}
	else
	{
		return;
	}
	suspectMenu->addSeparator();
	suspectMenu->addAction(ui.actionClear_All_Suspects);
	suspectMenu->addAction(ui.actionSave_Suspects);
	suspectMenu->addSeparator();

	suspectMenu->addAction(ui.actionShow_All_Suspects);
	suspectMenu->addAction(ui.actionHide_Old_Suspects);

	QPoint globalPos = event->globalPos();
	suspectMenu->popup(globalPos);
}

void NovaGUI::closeEvent(QCloseEvent * e)
{
	e = e;
	stopNova();
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
	homePath = GetHomePath();
	readPath = GetReadPath();
	writePath = GetWritePath();

	if((homePath == "") || (readPath == "") || (writePath == ""))
	{
		exit(1);
	}

	QDir::setCurrent((QString)homePath.c_str());

	novaComponents[COMPONENT_CE].name = "Classification Engine";
	novaComponents[COMPONENT_CE].terminalCommand = "gnome-terminal --disable-factory -t \"ClassificationEngine\" --geometry \"+0+600\" -x ClassificationEngine";
	novaComponents[COMPONENT_CE].noTerminalCommand = "nohup ClassificationEngine";

	novaComponents[COMPONENT_LM].name ="Local Traffic Monitor";
	novaComponents[COMPONENT_LM].terminalCommand ="gnome-terminal --disable-factory -t \"LocalTrafficMonitor\" --geometry \"+1000+0\" -x LocalTrafficMonitor";
	novaComponents[COMPONENT_LM].noTerminalCommand ="nohup LocalTrafficMonitor";

	novaComponents[COMPONENT_DM].name ="Doppelganger Module";
	novaComponents[COMPONENT_DM].terminalCommand ="gnome-terminal --disable-factory -t \"DoppelgangerModule\" --geometry \"+500+600\" -x DoppelgangerModule";
	novaComponents[COMPONENT_DM].noTerminalCommand ="nohup DoppelgangerModule";

	novaComponents[COMPONENT_DMH].name ="Doppelganger Honeyd";
	novaComponents[COMPONENT_DMH].terminalCommand ="gnome-terminal --disable-factory -t \"HoneyD Doppelganger\" --geometry \"+500+0\" -x sudo honeyd -d -i lo -f "+homePath+"/Config/doppelganger.config -p "+readPath+"/nmap-os-db -s /var/log/honeyd/honeydDoppservice.log 10.0.0.0/8";
	novaComponents[COMPONENT_DMH].noTerminalCommand ="nohup sudo honeyd -d -i lo -f "+homePath+"/Config/doppelganger.config -p "+readPath+"/nmap-os-db -s /var/log/honeyd/honeydDoppservice.log 10.0.0.0/8";

	novaComponents[COMPONENT_HS].name ="Haystack Module";
	novaComponents[COMPONENT_HS].terminalCommand ="gnome-terminal --disable-factory -t \"Haystack\" --geometry \"+1000+600\" -x Haystack",
	novaComponents[COMPONENT_HS].noTerminalCommand ="nohup Haystack > /dev/null";

	novaComponents[COMPONENT_HSH].name ="Haystack Honeyd";
	novaComponents[COMPONENT_HSH].terminalCommand ="gnome-terminal --disable-factory -t \"HoneyD Haystack\" --geometry \"+0+0\" -x sudo honeyd -d -i eth0 -f "+homePath+"/Config/haystack.config -p "+readPath+"/nmap-os-db -s /var/log/honeyd/honeydHaystackservice.log";
	novaComponents[COMPONENT_HSH].noTerminalCommand ="nohup sudo honeyd -d -i eth0 -f "+homePath+"/Config/haystack.config -p "+readPath+"/nmap-os-db -s /var/log/honeyd/honeydHaystackservice.log";
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

void NovaGUI::emitSystemStatusRefresh()
{
	emit refreshSystemStatus();
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
		pt.put<std::string>("IP", it->second.IP);
		pt.put<bool>("enabled", it->second.enabled);
		if(it->second.MAC.size())
			pt.put<std::string>("MAC", it->second.MAC);
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
void NovaGUI::writeHoneyd()
{
	stringstream out;
	stringstream doppelOut;

	vector<string> profilesParsed;

	for (ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
	{
		if (!it->second.parentProfile.compare(""))
		{
			string pString = profileToString(&it->second);
			out << pString;
			doppelOut << pString;
			profilesParsed.push_back(it->first);
		}
	}

	while (profilesParsed.size() < profiles.size())
	{
		for (ProfileTable::iterator it = profiles.begin(); it != profiles.end(); it++)
		{
			bool selfMatched = false;
			bool parentFound = false;
			for (uint i = 0; i < profilesParsed.size(); i++)
			{
				if(!it->second.parentProfile.compare(profilesParsed[i]))
				{
					parentFound = true;
					continue;
				}
				if (!it->first.compare(profilesParsed[i]))
				{
					selfMatched = true;
					break;
				}
			}

			if(!selfMatched && parentFound)
			{
				string pString = profileToString(&it->second);
				out << pString;
				doppelOut << pString;
				profilesParsed.push_back(it->first);

			}
		}
	}

	// Start node section
	out << endl << endl;
	for (NodeTable::iterator it = nodes.begin(); it != nodes.end(); it++)
	{
		if (!it->second.enabled)
		{
			continue;
		}

		switch (profiles[it->second.pfile].type)
		{
			case static_IP:
				out << "bind " << it->second.IP << " " << it->second.pfile << endl;
				break;
			case staticDHCP:
				out << "dhcp " << it->second.pfile << " on " << it->second.interface << " ethernet " << it->second.MAC << endl;
				break;
			case randomDHCP:
				out << "dhcp " << it->second.pfile << " on " << it->second.interface << endl;
				break;
			case Doppelganger:
				doppelOut << "bind " << it->second.IP << " " << it->second.pfile << endl;
				break;
		}
	}

	ofstream outFile(haystackPath.data());
	cout << "Saving to " << haystackPath.data() << endl;
	outFile << out.str() << endl;
	outFile.close();

	//cout << out.str() << endl;

	ofstream doppelOutFile(doppelgangerPath.data());
	doppelOutFile << doppelOut.str() << endl;
	doppelOutFile.close();
}

string NovaGUI::profileToString(profile* p)
{
	stringstream out;

	if (!p->parentProfile.compare("default") || !p->parentProfile.compare(""))
		out << "create " << p->name << endl;
	else
		out << "clone " << p->parentProfile << " " << p->name << endl;

	out << "set " << p->name  << " default tcp action " << p->tcpAction << endl;
	out << "set " << p->name  << " default udp action " << p->udpAction << endl;
	out << "set " << p->name  << " default icmp action " << p->icmpAction << endl;

	if (p->personality.compare(""))
		out << "set " << p->name << " personality \"" << p->personality << '"' << endl;

	if (p->ethernet.compare(""))
		out << "set " << p->name << " ethernet \"" << p->ethernet << '"' << endl;

	if (p->uptime.compare(""))
		out << "set " << p->name << " uptime " << p->uptime << endl;

	if (p->dropRate.compare(""))
		out << "set " << p->name << " droprate in " << p->dropRate << endl;

	for (uint i = 0; i < p->ports.size(); i++)
	{
		// Only include non-inherited ports
		if (!p->ports[i].second)
		{
			out << "add " << p->name;
			if(!ports[p->ports[i].first].type.compare("TCP"))
				out << " tcp port ";
			else
				out << " udp port ";
			out << ports[p->ports[i].first].portNum << " ";

			if (!(ports[p->ports[i].first].behavior.compare("script")))
			{
				string scriptName = ports[p->ports[i].first].scriptName;

				if (scripts[scriptName].path.compare(""))
					out << '"' << scripts[scriptName].path << '"'<< endl;
				else
					syslog(SYSL_ERR, "File: %s Line: %d Error writing profile port script %s: Path to script is null", __FILE__, __LINE__, scriptName.c_str());
			}
			else
			{
				out << ports[p->ports[i].first].behavior << endl;
			}
		}
	}

	out << endl;
	return out.str();
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
		syslog(SYSL_ERR, "File: %s Line: %d Problem loading scripts: %s", __FILE__, __LINE__, string(e.what()).c_str());
		prompter->DisplayPrompt(HONEYD_READ_FAIL, "Problem loading scripts:  " + string(e.what()));
	}
}

void NovaGUI::initiateSystemStatus()
{
	// Pull in the icons now that homePath is set
	string greenPath = homePath + "/Images/greendot.png";
	string yellowPath = homePath + "/Images/yellowdot.png";
	string redPath = homePath + "/Images/reddot.png";

	greenIcon = new QIcon(QPixmap(QString::fromStdString(greenPath)));
	yellowIcon = new QIcon(QPixmap(QString::fromStdString(yellowPath)));
	redIcon = new QIcon(QPixmap(QString::fromStdString(redPath)));

	// Populate the System Status table with empty widgets
	for (int i = 0; i < ui.systemStatusTable->rowCount(); i++)
		for (int j = 0; j < ui.systemStatusTable->columnCount(); j++)
			ui.systemStatusTable->setItem(i, j,  new QTableWidgetItem());

	// Add labels for our components
	ui.systemStatusTable->item(COMPONENT_CE,0)->setText(QString::fromStdString(novaComponents[COMPONENT_CE].name));
	ui.systemStatusTable->item(COMPONENT_LM,0)->setText(QString::fromStdString(novaComponents[COMPONENT_LM].name));
	ui.systemStatusTable->item(COMPONENT_DM,0)->setText(QString::fromStdString(novaComponents[COMPONENT_DM].name));
	ui.systemStatusTable->item(COMPONENT_DMH,0)->setText(QString::fromStdString(novaComponents[COMPONENT_DMH].name));
	ui.systemStatusTable->item(COMPONENT_HS,0)->setText(QString::fromStdString(novaComponents[COMPONENT_HS].name));
	ui.systemStatusTable->item(COMPONENT_HSH,0)->setText(QString::fromStdString(novaComponents[COMPONENT_HSH].name));
}


void NovaGUI::updateSystemStatus()
{
	QTableWidgetItem *item;
	QTableWidgetItem *pidItem;

	for (uint i = 0; i < sizeof(novaComponents)/sizeof(novaComponents[0]); i++)
	{
		item = ui.systemStatusTable->item(i,0);
		pidItem = ui.systemStatusTable->item(i,1);

		if (novaComponents[i].process == NULL || !novaComponents[i].process->pid())
		{
			item->setIcon(*redIcon);
			pidItem->setText("");
		}
		else
		{
			item->setIcon(*greenIcon);
			pidItem->setText(QString::number(novaComponents[i].process->pid()));
		}
	}

	on_systemStatusTable_itemSelectionChanged();
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
		syslog(SYSL_ERR, "File: %s Line: %d Problem loading ports: %s", __FILE__, __LINE__, string(e.what()).c_str());
		prompter->DisplayPrompt(HONEYD_READ_FAIL, "Problem loading ports: " + string(e.what()));
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
						syslog(SYSL_ERR, "File: %s Line: %d Problem loading nodes: %s", __FILE__, __LINE__, string(e.what()).c_str());
						prompter->DisplayPrompt(HONEYD_READ_FAIL, "Problem loading nodes: " + string(e.what()));
					}
				}
				catch(std::exception &e)
				{
					syslog(SYSL_ERR, "File: %s Line: %d Problem loading subnets: %s", __FILE__, __LINE__, string(e.what()).c_str());
					prompter->DisplayPrompt(HONEYD_READ_FAIL, "Problem loading subnets: " + string(e.what()));
				}
			}
		}
	}
	catch(std::exception &e)
	{
		syslog(SYSL_ERR, "File: %s Line: %d Problem loading group: %s - %s", __FILE__, __LINE__, group.c_str(), string(e.what()).c_str());
		prompter->DisplayPrompt(HONEYD_READ_FAIL, "Problem loading group:  string(e.what())");
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
				syslog(SYSL_ERR, "File: %s Line: %d Unexpected Entry in file: %s", __FILE__, __LINE__, string(v.first.data()).c_str());
				prompter->DisplayPrompt(UNEXPECTED_ENTRY, string(v.first.data()).c_str());
			}
		}
	}
	catch(std::exception &e)
	{
		syslog(SYSL_ERR, "File: %s Line: %d Problem loading subnets: %s", __FILE__, __LINE__, string(e.what()).c_str());
		prompter->DisplayPrompt(HONEYD_LOAD_FAIL, "Problem loading subnets: " + string(e.what()));
	}
}


//loads haystack nodes from file for current group
void NovaGUI::loadNodes(ptree *ptr)
{
	profile p;
	//ptree * ptr2;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			if(!string(v.first.data()).compare("node"))
			{
				node n;
				int max = 0;
				bool unique = true;
				stringstream ss;
				uint i = 0, j = 0;
				j = ~j; // 2^32-1

				n.tree = v.second;
				//Required xml entires
				n.interface = v.second.get<std::string>("interface");
				n.IP = v.second.get<std::string>("IP");
				n.enabled = v.second.get<bool>("enabled");
				n.pfile = v.second.get<std::string>("profile.name");
				p = profiles[n.pfile];

				//Get mac if present
				try //Conditional: has "set" values
				{
					//ptr2 = &v.second.get_child("MAC");
					//pass 'set' subset and pointer to this profile
					n.MAC = v.second.get<std::string>("MAC");
				}
				catch(...){}

				switch(p.type)
				{

				//***** STATIC IP ********//
				case static_IP:

					n.name = n.IP;
					//intialize subnet to NULL and check for smallest bounding subnet
					n.sub = "";
					n.realIP = htonl(inet_addr(n.IP.c_str())); //convert ip to uint32
					//Tracks the mask with smallest range by comparing num of bits used.

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
							if(it->second.address.compare(n.IP))
							{
								max = it->second.maskBits;
								n.sub = it->second.address;
							}
						}
					}

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
						nodes[n.name] = n;

						//Put address of saved node in subnet's list of nodes.
						subnets[nodes[n.name].sub].nodes.push_back(n.name);
					}
					//If no subnet found, can't use node unless it's doppelganger.
					else
					{
						syslog(SYSL_ERR, "File: %s Line: %d Node at IP: %s is outside all valid subnet ranges", __FILE__, __LINE__, n.IP.c_str());
						prompter->DisplayPrompt(HONEYD_INVALID_SUBNET, " Node at IP is outside all valid subnet ranges: " + n.name);
					}
					break;


				//***** STATIC DHCP (static MAC) ********//
				case staticDHCP:

					//If no MAC is set, there's a problem
					if(!n.MAC.size())
					{
						syslog(SYSL_ERR, "File: %s Line: %d DHCP Enabled node using profile %s does not have a MAC Address.",
								__FILE__, __LINE__, string(n.pfile).c_str());
						continue;
					}

					//Associated MAC is already in use, this is not allowed, throw out the node
					//TODO proper debug prints and popups needed
					if(nodes.find(n.MAC) != nodes.end())
					{
						syslog(SYSL_ERR, "File: %s Line: %d Duplicate MAC address detected in nodes: %s", __FILE__, __LINE__, n.MAC.c_str());
						continue;
					}
					n.name = n.MAC;
					n.sub = "";
					for(SubnetTable::iterator it = subnets.begin(); it != subnets.end(); it++)
					{
						if(!it->second.name.compare(n.interface))
							n.sub = it->second.address;
					}
					// If no valid subnet/interface found
					//TODO proper debug prints and popups needed
					if(!n.sub.compare(""))
					{
						syslog(SYSL_ERR, "File: %s Line: %d DHCP Enabled Node with MAC: %s is unable to resolve it's interface.", __FILE__, __LINE__, n.MAC.c_str());
						continue;
					}

					//save the node in the table
					nodes[n.name] = n;

					//Put address of saved node in subnet's list of nodes.
					subnets[nodes[n.name].sub].nodes.push_back(n.name);
					break;

				//***** RANDOM DHCP (random MAC each time run) ********//
				case randomDHCP:

					n.name = n.pfile + " on " + n.interface;

					//Finds a unique identifier
					while((nodes.find(n.name) != nodes.end()) && (i < j))
					{
						i++;
						ss.str("");
						ss << n.pfile << " on " << n.interface << "-" << i;
						n.name = ss.str();
					}
					n.sub = "";
					for(SubnetTable::iterator it = subnets.begin(); it != subnets.end(); it++)
					{
						if(!it->second.name.compare(n.interface))
							n.sub = it->second.address;
					}
					// If no valid subnet/interface found
					//TODO proper debug prints and popups needed
					if(!n.sub.compare(""))
					{
						syslog(SYSL_ERR, "File: %s Line: %d DHCP Enabled Node is unable to resolve it's interface: %s.", __FILE__, __LINE__, n.interface.c_str());
						continue;
					}
					//save the node in the table
					nodes[n.name] = n;

					//Put address of saved node in subnet's list of nodes.
					subnets[nodes[n.name].sub].nodes.push_back(n.name);
					break;

				//***** Doppelganger ********//
				case Doppelganger:
					n.name = "Doppelganger";
					n.sub = "";
					for(SubnetTable::iterator it = subnets.begin(); it != subnets.end(); it++)
					{
						if(!it->second.name.compare(n.interface))
							n.sub = it->second.address;
					}
					// If no valid subnet/interface found
					//TODO proper debug prints and popups needed
					if(!n.sub.compare(""))
					{
						syslog(SYSL_ERR, "File: %s Line: %d The Doppelganger is unable to resolve the interface: %s", __FILE__, __LINE__, n.interface.c_str());
						continue;
					}
					//save the node in the table
					nodes[n.name] = n;

					//Put address of saved node in subnet's list of nodes.
					subnets[nodes[n.name].sub].nodes.push_back(n.name);
					break;
				}
			}
			else
			{
				syslog(SYSL_ERR, "File: %s Line: %d Unexpected Entry in file: %s", __FILE__, __LINE__, string(v.first.data()).c_str());
				prompter->DisplayPrompt(UNEXPECTED_ENTRY, "Unexpected Entry in file: " + string(v.first.data()));
			}
		}
	}
	catch(std::exception &e)
	{
		syslog(SYSL_ERR, "File: %s Line: %d Problem loading nodes: %s", __FILE__, __LINE__, string(e.what()).c_str());
		prompter->DisplayPrompt(HONEYD_LOAD_FAIL, "Problem loading nodes: " + string(e.what()));
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
				p.ports.clear();
				p.type = (profileType)v.second.get<int>("type");
				for(uint i = 0; i < INHERITED_MAX; i++)
				{
					p.inherited[i] = false;
				}

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
				syslog(SYSL_ERR, "File: %s Line: %d Invalid XML Path %s", __FILE__, __LINE__, string(v.first.data()).c_str());
			}
		}
	}
	catch(std::exception &e)
	{
		syslog(SYSL_ERR, "File: %s Line: %d Problem loading Profiles: %s", __FILE__, __LINE__, string(e.what()).c_str());
		prompter->DisplayPrompt(HONEYD_LOAD_FAIL, "Problem loading Profiles: " + string(e.what()));
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
				p->inherited[TCP_ACTION] = false;
				continue;
			}
			prefix = "UDP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->udpAction = v.second.data();
				p->inherited[UDP_ACTION] = false;
				continue;
			}
			prefix = "ICMP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->icmpAction = v.second.data();
				p->inherited[ICMP_ACTION] = false;
				continue;
			}
			prefix = "personality";
			if(!string(v.first.data()).compare(prefix))
			{
				p->personality = v.second.data();
				p->inherited[PERSONALITY] = false;
				continue;
			}
			prefix = "ethernet";
			if(!string(v.first.data()).compare(prefix))
			{
				p->ethernet = v.second.data();
				p->inherited[ETHERNET] = false;
				continue;
			}
			prefix = "uptime";
			if(!string(v.first.data()).compare(prefix))
			{
				p->uptime = v.second.data();
				p->inherited[UPTIME] = false;
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
				p->inherited[DROP_RATE] = false;
				continue;
			}
		}
	}
	catch(std::exception &e)
	{
		syslog(SYSL_ERR, "File: %s Line: %d Problem loading profile set parameters: %s", __FILE__, __LINE__, string(e.what()).c_str());
		prompter->DisplayPrompt(HONEYD_LOAD_FAIL, "Problem loading profile set parameters: " + string(e.what()));
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
		for(uint i = 0; i < p->ports.size(); i++)
		{
			p->ports[i].second = true;
		}
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
						if(!prt->portNum.compare(ports[p->ports[i].first].portNum) && !prt->type.compare(ports[p->ports[i].first].type))
						{
							p->ports.erase(p->ports.begin()+i);
						}
					}
					//Add specified port
					pair<string, bool> portPair;
					portPair.first = prt->portName;
					portPair.second = false;
					if(!p->ports.size())
						p->ports.push_back(portPair);
					else
					{
						uint i = 0;
						for(i = 0; i < p->ports.size(); i++)
						{
							port * temp = &ports[p->ports[i].first];
							if((atoi(temp->portNum.c_str())) < (atoi(prt->portNum.c_str())))
							{
								continue;
							}
							break;
						}
						if(i < p->ports.size())
							p->ports.insert(p->ports.begin()+i, portPair);
						else
							p->ports.push_back(portPair);
					}
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
		syslog(SYSL_ERR, "File: %s Line: %d Problem loading profile add parameters: %s", __FILE__, __LINE__, string(e.what()).c_str());
		prompter->DisplayPrompt(HONEYD_LOAD_FAIL, "Problem loading profile add parameters: " + string(e.what()));
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

			for(uint i = 0; i < INHERITED_MAX; i++)
			{
				prof.inherited[i] = true;
			}

			try //Conditional: If profile overrides type
			{
				prof.type = (profileType)v.second.get<int>("type");
				prof.inherited[TYPE] = false;
			}
			catch(...){}

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
		syslog(SYSL_ERR, "File: %s Line: %d Problem loading sub profiles: %s", __FILE__, __LINE__, string(e.what()).c_str());
		prompter->DisplayPrompt(HONEYD_LOAD_FAIL, "Problem loading sub profiles:" + string(e.what()));
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
		syslog(SYSL_ERR, "File: %s Line: %d accept: %s", __FILE__, __LINE__, strerror(errno));
		sclose(connectionSocket);
		return false;
	}
	if((bytesRead = recv(connectionSocket, buf, MAX_MSG_SIZE, 0 )) == 0)
	{
		return false;
	}
	else if(bytesRead == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d recv: %s", __FILE__, __LINE__, strerror(errno));
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
		syslog(SYSL_ERR, "File: %s Line: %d Error interpreting received Suspect: %s", __FILE__, __LINE__, string(e.what()).c_str());
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
	editingSuspectList = true;
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
	editingSuspectList = false;
}

//Updates the UI with the latest suspect information
//*NOTE This slot is not thread safe, make sure you set appropriate locks before sending a signal to this slot
void NovaGUI::drawSuspect(in_addr_t suspectAddr)
{
	editingSuspectList = true;
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
	editingSuspectList = false;
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

//Clears the suspect tables completely.
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
	if(!runAsWindowUp)
	{
		Run_Popup *w = new Run_Popup(this, homePath);
		w->show();
		runAsWindowUp = true;
	}
}

void NovaGUI::on_actionStopNova_triggered()
{
	stopNova();

	// Were we in training mode?
	if (isTraining)
	{
		prompter->DisplayPrompt(LAUNCH_TRAINING_MERGE,
			"ClassificationEngine was in training mode. Would you like to merge the capture file into the training database now?",
			ui.actionTrainingData, NULL);
	}
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
	stopNova();
	exit(1);
}

void NovaGUI::on_actionClear_All_Suspects_triggered()
{
	editingSuspectList = true;
	clearSuspects();
	drawAllSuspects();
	editingSuspectList = false;
}

void NovaGUI::on_actionClear_Suspect_triggered()
{
	QListWidget * list;
	if(ui.suspectList->hasFocus())
	{
		list = ui.suspectList;
	}
	else if(ui.hostileList->hasFocus())
	{
		list = ui.hostileList;
	}
	if(list->currentItem() != NULL && list->isItemSelected(list->currentItem()))
	{
		string suspectStr = list->currentItem()->text().toStdString();
		in_addr_t addr = inet_addr(suspectStr.c_str());
		hideSuspect(addr);
		clearSuspect(suspectStr);
	}
}

void NovaGUI::on_actionHide_Suspect_triggered()
{
	QListWidget * list;
	if(ui.suspectList->hasFocus())
	{
		list = ui.suspectList;
	}
	else if(ui.hostileList->hasFocus())
	{
		list = ui.hostileList;
	}
	if(list->currentItem() != NULL && list->isItemSelected(list->currentItem()))
	{
		in_addr_t addr = inet_addr(list->currentItem()->text().toStdString().c_str());
		hideSuspect(addr);
	}
}

void NovaGUI::on_actionSave_Suspects_triggered()
{
	saveSuspects();
}

void NovaGUI::on_actionMakeDataFile_triggered()
{
	 QString data = QFileDialog::getOpenFileName(this,
			 tr("File to select classifications from"), QString::fromStdString(trainingDataPath), tr("NOVA Classification Database (*.db)"));

	if (data.isNull())
		return;

	trainingSuspectMap* map = ParseTrainingDb(data.toStdString());
	if (map == NULL)
	{
		prompter->DisplayPrompt(CONFIG_READ_FAIL, "Error parsing file " + data.toStdString());
		return;
	}


	classifierPrompt* classifier = new classifierPrompt(map);

	if (classifier->exec() == QDialog::Rejected)
		return;

	string dataFileContent = MakaDataFile(*map);

	ofstream out (trainingDataPath.data());
	out << dataFileContent;
	out.close();
}

void NovaGUI::on_actionTrainingData_triggered()
{
	 QString data = QFileDialog::getOpenFileName(this,
			 tr("Classification Engine Data Dump"), QString::fromStdString(trainingDataPath), tr("NOVA Classification Dump (*.dump)"));

	if (data.isNull())
		return;

	trainingDumpMap* trainingDump = ParseEngineCaptureFile(data.toStdString());

	if (trainingDump == NULL)
	{
		prompter->DisplayPrompt(CONFIG_READ_FAIL, "Error parsing file " + data.toStdString());
		return;
	}

	ThinTrainingPoints(trainingDump, 0.001);

	classifierPrompt* classifier = new classifierPrompt(trainingDump);

	if (classifier->exec() == QDialog::Rejected)
		return;

	trainingSuspectMap* headerMap = classifier->getStateData();

	QString outputFile = QFileDialog::getSaveFileName(this,
			tr("Classification Database File"), QString::fromStdString(trainingDataPath), tr("NOVA Classification Database (*.db)"));

	if (outputFile.isNull())
		return;

	if (!CaptureToTrainingDb(outputFile.toStdString(), headerMap))
	{
		prompter->DisplayPrompt(CONFIG_READ_FAIL, "Error parsing the input files. Please see the logs for more details.");
	}
}

void  NovaGUI::on_actionHide_Old_Suspects_triggered()
{
	editingSuspectList = true;
	clearSuspectList();
	editingSuspectList = false;
}

void  NovaGUI::on_actionShow_All_Suspects_triggered()
{
	editingSuspectList = true;
	drawAllSuspects();
	editingSuspectList = false;
}

void NovaGUI::on_actionHelp_2_triggered()
{
	if(!isHelpUp)
	{
		Nova_Manual *wi = new Nova_Manual(this);
		wi->show();
		isHelpUp = true;
	}
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
	// TODO: Put this back? It was really annoying if you had an existing
	// haystack.config you wanted to use, kept rewriting it on start.
	// Commented for now until the Node setup works in the GUI.
	//writeHoneyd();
	startNova();
}
void NovaGUI::on_stopButton_clicked()
{
	emit on_actionStopNova_triggered();
}

void NovaGUI::on_systemStatusTable_itemSelectionChanged()
{
	int row = ui.systemStatusTable->currentRow();

	if (novaComponents[row].process != NULL && novaComponents[row].process->pid())
	{
		ui.systemStatStartButton->setDisabled(true);
		ui.systemStatKillButton->setDisabled(false);

		// We can't send a stop signal to honeyd, force using the kill button
		if (row == COMPONENT_DMH || row == COMPONENT_HSH)
			ui.systemStatStopButton->setDisabled(true);
		else
			ui.systemStatStopButton->setDisabled(false);

	}
	else
	{
		ui.systemStatStartButton->setDisabled(false);
		ui.systemStatStopButton->setDisabled(true);
		ui.systemStatKillButton->setDisabled(true);
	}
}

void NovaGUI::on_systemStatStartButton_clicked()
{
	int row = ui.systemStatusTable->currentRow();

	switch (row) {
	case COMPONENT_CE:
		startComponent(&novaComponents[COMPONENT_CE]);
		break;
	case COMPONENT_DM:
		startComponent(&novaComponents[COMPONENT_DM]);
		break;
	case COMPONENT_HS:
		startComponent(&novaComponents[COMPONENT_HS]);
		break;
	case COMPONENT_LM:
		startComponent(&novaComponents[COMPONENT_LM]);
		break;
	case COMPONENT_HSH:
		startComponent(&novaComponents[COMPONENT_HSH]);
		break;
	case COMPONENT_DMH:
		startComponent(&novaComponents[COMPONENT_DMH]);
		break;
	}

	updateSystemStatus();
}

void NovaGUI::on_systemStatKillButton_clicked()
{
	QProcess *process = novaComponents[ui.systemStatusTable->currentRow()].process;

	// Fix for honeyd not closing with gnome-terminal + sudo
	if (useTerminals && process != NULL && process->pid() &&
			(ui.systemStatusTable->currentRow() == COMPONENT_DMH || ui.systemStatusTable->currentRow() == COMPONENT_HSH))
	{
		QString killString = QString("sudo pkill -TERM -P ") + QString::number(process->pid());
		system(killString.toStdString().c_str());

		killString = QString("sudo kill ") + QString::number(process->pid());
		system(killString.toStdString().c_str());
	}

	// Politely ask the process to die
	if(process != NULL && process->pid())
		process->terminate();

	// Tell the process to die in a stern voice
	if(process != NULL && process->pid())
		process->kill();

	// Give up telling it to die and kill it ourselves with the power of root
	if(process != NULL && process->pid())
	{
		QString killString = QString("sudo kill ") + QString::number(process->pid());
		system(killString.toStdString().c_str());
	}
}

void NovaGUI::on_systemStatStopButton_clicked()
{
	int row = ui.systemStatusTable->currentRow();

	//Sets the message
	message.SetMessage(EXIT);
	msgLen = message.SerialzeMessage(msgBuffer);

	switch (row) {
	case COMPONENT_CE:
		sendToCE();
		break;
	case COMPONENT_DM:
		sendToDM();
		break;
	case COMPONENT_HS:
		sendToHS();
		break;
	case COMPONENT_LM:
		sendToLTM();
		break;
	case COMPONENT_DMH:
		if (novaComponents[COMPONENT_DMH].process != NULL && novaComponents[COMPONENT_DMH].process->pid() != 0)
		{
			QString killString = QString("sudo pkill -TERM -P ") + QString::number(novaComponents[COMPONENT_DMH].process->pid());
			system(killString.toStdString().c_str());

			killString = QString("sudo kill ") + QString::number(novaComponents[COMPONENT_DMH].process->pid());
			system(killString.toStdString().c_str());
		}
		break;
	case COMPONENT_HSH:
		if (novaComponents[COMPONENT_HSH].process != NULL && novaComponents[COMPONENT_HSH].process->pid() != 0)
		{
			QString killString = QString("sudo pkill -TERM -P ") + QString::number(novaComponents[COMPONENT_HSH].process->pid());
			system(killString.toStdString().c_str());

			killString = QString("sudo kill ") + QString::number(novaComponents[COMPONENT_HSH].process->pid());
			system(killString.toStdString().c_str());
		}
		break;
	}
}

void NovaGUI::on_clearSuspectsButton_clicked()
{
	editingSuspectList = true;
	clearSuspects();
	drawAllSuspects();
	editingSuspectList = false;
}


/************************************************
 * List Signal Handlers
 ************************************************/
void NovaGUI::on_suspectList_itemSelectionChanged()
{
	if(!editingSuspectList)
	{
		pthread_rwlock_wrlock(&lock);
		if(ui.suspectList->currentItem() != NULL)
		{
			in_addr_t addr = inet_addr(ui.suspectList->currentItem()->text().toStdString().c_str());
			ui.suspectFeaturesEdit->setText(QString(SuspectTable[addr].suspect->ToString(featureEnabled).c_str()));
		}
		pthread_rwlock_unlock(&lock);
	}
}

/************************************************
 * IPC Functions
 ************************************************/
void NovaGUI::hideSuspect(in_addr_t addr)
{
	pthread_rwlock_wrlock(&lock);
	editingSuspectList = true;
	suspectItem * sItem = &SuspectTable[addr];
	if(!sItem->item->isSelected())
	{
		pthread_rwlock_unlock(&lock);
		editingSuspectList = false;
		return;
	}
	ui.suspectList->removeItemWidget(sItem->item);
	delete sItem->item;
	sItem->item = NULL;
	if(sItem->mainItem != NULL)
	{
		ui.hostileList->removeItemWidget(sItem->mainItem);
		delete sItem->mainItem;
		sItem->mainItem = NULL;
	}
	pthread_rwlock_unlock(&lock);
	editingSuspectList = false;
}

/*********************************************************
 ----------------- General Functions ---------------------
 *********************************************************/

namespace Nova
{

/************************************************
 * Thread Loops
 ************************************************/

void *StatusUpdate(void *ptr)
{
	while(true)
	{
		((NovaGUI*)ptr)->emitSystemStatusRefresh();

		sleep(2);
	}
	return NULL;
}

void *CEListen(void *ptr)
{
	while(true)
	{
		((NovaGUI*)ptr)->receiveCE(CE_InSock);
	}
	return NULL;
}

//Removes all information on a suspect
void clearSuspect(string suspectStr)
{
	pthread_rwlock_wrlock(&lock);
	SuspectTable.erase(inet_addr(suspectStr.c_str()));
	message.SetMessage(CLEAR_SUSPECT, suspectStr);
	msgLen = message.SerialzeMessage(msgBuffer);
	sendAll();
	pthread_rwlock_unlock(&lock);
}

//Deletes all Suspect information for the GUI and Nova
void clearSuspects()
{
	pthread_rwlock_wrlock(&lock);
	SuspectTable.clear();
	message.SetMessage(CLEAR_ALL);
	msgLen = message.SerialzeMessage(msgBuffer);
	sendAll();
	pthread_rwlock_unlock(&lock);
}

void stopNova()
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

		if (line != NULL)
		{
			string cmd = "sudo kill " + string(line);
			if(cmd.size() > 5)
				system(cmd.c_str());
		}
	}
	pclose(out);
}


void startNova()
{

	string homePath = GetHomePath();
	string input = homePath + "/Config/NOVAConfig.txt";

	// Reload the configuration file
	configuration.LoadConfig(configurationFile.c_str(), homePath, __FILE__);

	useTerminals = atoi(configuration.options["USE_TERMINALS"].data.c_str());
	isTraining = atoi(configuration.options["IS_TRAINING"].data.c_str());

	string enabledFeatureMask = configuration.options["ENABLED_FEATURES"].data;


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

	// Start and processes that aren't running already
	for (uint i = 0; i < sizeof(novaComponents)/sizeof(novaComponents[0]); i++)
	{
		if(novaComponents[i].process == NULL || !novaComponents[i].process->pid())
			startComponent(&novaComponents[i]);
	}

}

void startComponent(novaComponent *component)
{
	QString program;

	if (useTerminals)
		program = QString::fromStdString(component->terminalCommand);
	else
		program = QString::fromStdString(component->noTerminalCommand);

	// Is the process already running?
	if (component->process != NULL)
	{
		component->process->kill();
		delete component->process;
	}

	component->process = new QProcess();
	component->process->start(program);
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
		syslog(SYSL_ERR, "File: %s Line: %d socket: %s", __FILE__, __LINE__, strerror(errno));
		close(CE_OutSock);
		exit(1);
	}
	//Sends the message
	len = strlen(CE_OutAddress.sun_path) + sizeof(CE_OutAddress.sun_family);
	if (connect(CE_OutSock, (struct sockaddr *)&CE_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d connect: %s", __FILE__, __LINE__, strerror(errno));
		close(CE_OutSock);
		return;
	}

	else if (send(CE_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d send: %s", __FILE__, __LINE__, strerror(errno));
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
		syslog(SYSL_ERR, "File: %s Line: %d socket: %s", __FILE__, __LINE__, strerror(errno));
		close(DM_OutSock);
		exit(1);
	}
	//Sends the message
	len = strlen(DM_OutAddress.sun_path) + sizeof(DM_OutAddress.sun_family);
	if (connect(DM_OutSock, (struct sockaddr *)&DM_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d connect: %s", __FILE__, __LINE__, strerror(errno));
		close(DM_OutSock);
		return;
	}

	else if (send(DM_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d send: %s", __FILE__, __LINE__, strerror(errno));
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
		syslog(SYSL_ERR, "File: %s Line: %d socket: %s", __FILE__, __LINE__, strerror(errno));
		close(HS_OutSock);
		exit(1);
	}
	//Sends the message
	len = strlen(HS_OutAddress.sun_path) + sizeof(HS_OutAddress.sun_family);
	if (connect(HS_OutSock, (struct sockaddr *)&HS_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d connect: %s", __FILE__, __LINE__, strerror(errno));
		close(HS_OutSock);
		return;
	}

	else if (send(HS_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d send: %s", __FILE__, __LINE__, strerror(errno));
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
		syslog(SYSL_ERR, "File: %s Line: %d socket: %s", __FILE__, __LINE__, strerror(errno));
		close(LTM_OutSock);
		exit(1);
	}
	//Sends the message
	len = strlen(LTM_OutAddress.sun_path) + sizeof(LTM_OutAddress.sun_family);
	if (connect(LTM_OutSock, (struct sockaddr *)&LTM_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d connect: %s", __FILE__, __LINE__, strerror(errno));
		close(LTM_OutSock);
		return;
	}

	else if (send(LTM_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d send: %s", __FILE__, __LINE__, strerror(errno));
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
		syslog(SYSL_ERR, "File: %s Line: %d socket: %s", __FILE__, __LINE__, strerror(errno));
		close(CE_OutSock);
	}

	//DM OUT -------------------------------------------------
	if ((DM_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d socket: %s", __FILE__, __LINE__, strerror(errno));
		close(DM_OutSock);
	}

	//HS OUT -------------------------------------------------
	if ((HS_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d socket: %s", __FILE__, __LINE__, strerror(errno));
		close(HS_OutSock);
	}

	//LTM OUT ------------------------------------------------
	if ((LTM_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d socket: %s", __FILE__, __LINE__, strerror(errno));
		close(LTM_OutSock);
	}


	//Sends the message
	//CE OUT -------------------------------------------------
	len = strlen(CE_OutAddress.sun_path) + sizeof(CE_OutAddress.sun_family);
	if (connect(CE_OutSock, (struct sockaddr *)&CE_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d connect: %s", __FILE__, __LINE__, strerror(errno));
		close(CE_OutSock);
	}

	else if (send(CE_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d send: %s", __FILE__, __LINE__, strerror(errno));
		close(CE_OutSock);
	}
	close(CE_OutSock);
	// -------------------------------------------------------

	//DM OUT -------------------------------------------------
	len = strlen(DM_OutAddress.sun_path) + sizeof(DM_OutAddress.sun_family);
	if (connect(DM_OutSock, (struct sockaddr *)&DM_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d connect: %s", __FILE__, __LINE__, strerror(errno));
		close(DM_OutSock);
	}

	else if (send(DM_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d send: %s", __FILE__, __LINE__, strerror(errno));
		close(DM_OutSock);
	}
	close(DM_OutSock);
	// -------------------------------------------------------


	//HS OUT -------------------------------------------------
	len = strlen(HS_OutAddress.sun_path) + sizeof(HS_OutAddress.sun_family);
	if (connect(HS_OutSock, (struct sockaddr *)&HS_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d connect: %s", __FILE__, __LINE__, strerror(errno));
		close(HS_OutSock);
	}

	else if (send(HS_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d send: %s", __FILE__, __LINE__, strerror(errno));
		close(HS_OutSock);
	}
	close(HS_OutSock);
	// -------------------------------------------------------


	//LTM OUT ------------------------------------------------
	len = strlen(LTM_OutAddress.sun_path) + sizeof(LTM_OutAddress.sun_family);
	if (connect(LTM_OutSock, (struct sockaddr *)&LTM_OutAddress, len) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d connect: %s", __FILE__, __LINE__, strerror(errno));
		close(LTM_OutSock);
	}

	else if (send(LTM_OutSock, msgBuffer, msgLen, 0) == -1)
	{
		syslog(SYSL_ERR, "File: %s Line: %d send: %s", __FILE__, __LINE__, strerror(errno));
		close(LTM_OutSock);
	}
	close(LTM_OutSock);
	// -------------------------------------------------------
}

void sclose(int sock)
{
	close(sock);
}

}
