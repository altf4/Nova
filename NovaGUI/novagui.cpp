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
#include <errno.h>

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

pthread_rwlock_t lock;

LoggerPtr m_logger(Logger::getLogger("main"));

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
	DOMConfigurator::configure("Config/Log4cxxConfig.xml");
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

bool NovaGUI::receiveCE(int socket)
{
	struct sockaddr_un remote;
	int socketSize, connectionSocket;
	int bytesRead;
	char buf[MAX_MSG_SIZE];

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

	Suspect* suspect = new Suspect();

	sclose(connectionSocket);
	try
	{
		stringstream ss;
		ss << buf;
		boost::archive::text_iarchive ia(ss);
		// create and open an archive for input
		// read class state from archive
		ia >> suspect;
		// archive and stream closed when destructors are called

	}
	catch(boost::archive::archive_exception e)
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

void NovaGUI::on_actionRunNova_triggered()
{
	startNova();
}

void NovaGUI::on_actionRunNovaAs_triggered()
{
	Run_Popup *w = new Run_Popup(this);
	w->show();
}

void NovaGUI::on_actionStopNova_triggered()
{
	closeNova();
}

void NovaGUI::on_actionConfigure_triggered()
{
	NovaConfig *w = new NovaConfig(this);
	w->show();
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
		ifstream * config = new ifstream("Config/NOVAConfig.txt");
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
			system("nohup honeyd -d -i eth0 -f Config/haystack.config -p $HOME/Programs/nmap-4.11/nmap-os-fingerprints"
					" -s Config/honeydservice.log > /dev/null &");
			system("nohup honeyd -d -i lo -f Config/doppelganger.config -p $HOME/Programs/nmap-4.11/nmap-os-fingerprints"
					" -s Config/honeydDoppservice.log 10.0.0.0/8 > /dev/null &");
			system("nohup ./bin/LocalTrafficMonitor -n Config/NOVAConfig.txt -l Config/Log4cxxConfig.xml > /dev/null &");
			system("nohup ./bin/Haystack -n Config/NOVAConfig.txt -l Config/Log4cxxConfig.xml > /dev/null &");
			system("nohup ./bin/ClassificationEngine -n Config/NOVAConfig.txt -l Config/Log4cxxConfig.xml > /dev/null &");
			system("nohup ./bin/DoppelgangerModule -n Config/NOVAConfig.txt -l Config/Log4cxxConfig.xml > /dev/null &");
		}
		else
		{
			system("(gnome-terminal -t \"HoneyD Haystack\" --geometry \"+0+0\" -x honeyd -d -i eth0 -f Config/haystack.config"
					" -p $HOME/Programs/nmap-4.11/nmap-os-fingerprints -s Config/honeydservice.log )&");
			system("(gnome-terminal -t \"HoneyD Doppelganger\" --geometry \"+500+0\" -x honeyd -d -i lo -f Config/doppelganger.config"
					" -p $HOME/Programs/nmap-4.11/nmap-os-fingerprints -s Config/honeydDoppservice.log 10.0.0.0/8 )&");
			system("(gnome-terminal -t \"LocalTrafficMonitor\" --geometry \"+1000+0\" -x ./bin/LocalTrafficMonitor"
					" -n Config/NOVAConfig.txt -l Config/Log4cxxConfig_Console.xml)&");
			system("(gnome-terminal -t \"Haystack\" --geometry \"+1000+600\" -x ./bin/Haystack -n Config/NOVAConfig.txt"
					" -l Config/Log4cxxConfig_Console.xml)&");
			system("(gnome-terminal -t \"ClassificationEngine\" --geometry \"+0+600\" -x ./bin/ClassificationEngine"
					" -n Config/NOVAConfig.txt -l Config/Log4cxxConfig_Console.xml)&");
			system("(gnome-terminal -t \"DoppelgangerModule\" --geometry \"+500+600\" -x ./bin/DoppelgangerModule"
					" -n Config/NOVAConfig.txt -l Config/Log4cxxConfig_Console.xml )&");
		}
		novaRunning = true;
	}
}

/*****************************************************************************
// ------------------------- SOCKET FUNCTIONS --------------------------------
*****************************************************************************/
void getSocketAddr()
{

	//CE IN --------------------------------------------------
	//Builds the key path
	string path = getenv("HOME");
	string key = CE_FILENAME;
	path += key;
	//Builds the address
	CE_InAddress.sun_family = AF_UNIX;
	strcpy(CE_InAddress.sun_path, path.c_str());
	unlink(CE_InAddress.sun_path);

	//CE OUT -------------------------------------------------
	//Builds the key path
	path = getenv("HOME");
	key = CE_GUI_FILENAME;
	path += key;
	//Builds the address
	CE_OutAddress.sun_family = AF_UNIX;
	strcpy(CE_OutAddress.sun_path, path.c_str());

	//DM OUT -------------------------------------------------
	//Builds the key path
	path = getenv("HOME");
	key = DM_GUI_FILENAME;
	path += key;
	//Builds the address
	DM_OutAddress.sun_family = AF_UNIX;
	strcpy(DM_OutAddress.sun_path, path.c_str());

	//HS OUT -------------------------------------------------
	//Builds the key path
	path = getenv("HOME");
	key = HS_GUI_FILENAME;
	path += key;
	//Builds the address
	HS_OutAddress.sun_family = AF_UNIX;
	strcpy(HS_OutAddress.sun_path, path.c_str());

	//LTM OUT ------------------------------------------------
	//Builds the key path
	path = getenv("HOME");
	key = LTM_GUI_FILENAME;
	path += key;
	//Builds the address
	LTM_OutAddress.sun_family = AF_UNIX;
	strcpy(LTM_OutAddress.sun_path, path.c_str());

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
		exit(1);
	}

	//DM OUT -------------------------------------------------
	if ((DM_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(DM_OutSock);
		exit(1);
	}

	//HS OUT -------------------------------------------------
	if ((HS_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(HS_OutSock);
		exit(1);
	}

	//LTM OUT ------------------------------------------------
	if ((LTM_OutSock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		LOG4CXX_ERROR(m_logger,"socket: " << strerror(errno));
		close(LTM_OutSock);
		exit(1);
	}


	//Sends the message
	//CE OUT -------------------------------------------------
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
	// -------------------------------------------------------

	//DM OUT -------------------------------------------------
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
	// -------------------------------------------------------


	//HS OUT -------------------------------------------------
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
	// -------------------------------------------------------


	//LTM OUT ------------------------------------------------
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
	// -------------------------------------------------------
}

void sclose(int sock)
{
	close(sock);
}
/*****************************************************************************
// ------------------------- END SOCKET FUNCTIONS ----------------------------
*****************************************************************************/
