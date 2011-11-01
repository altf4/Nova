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
	//Might need locks later
	pthread_rwlock_init(&lock, NULL);
	ui.setupUi(this);
	DOMConfigurator::configure("Config/Log4cxxConfig_GUI.xml");
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
	pthread_create(&CEListenThread,NULL,CEListen, this);
	pthread_create(&CEDrawThread,NULL,CEDraw, this);
}

NovaGUI::~NovaGUI()
{
	closeNova();
}


void *CEListen(void *ptr)
{
	while(true)
	{
		((NovaGUI*)ptr)->ReceiveCE(CE_InSock);
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

bool NovaGUI::ReceiveCE(int socket)
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
		return false;
	}
	struct suspectItem sItem;
	sItem.suspect = suspect;
	updateSuspect(sItem);
	return true;
}

void NovaGUI::updateSuspect(suspectItem suspectItem)
{

	if(suspectItem.suspect == NULL) return;

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
			suspectItem.suspect->needs_classification_update = false;
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

		//Update the entry in the table
		SuspectTable[suspectItem.suspect->IP_address.s_addr] = suspectItem;
	}
	//If the suspect is new
	else
	{
		//This flag is set because it is a new item and to the GUI display has no classification yet
		suspectItem.suspect->needs_classification_update = true;
		suspectItem.item = NULL;

		//Put it in the table
		SuspectTable[suspectItem.suspect->IP_address.s_addr] = suspectItem;
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

	QListWidgetItem * item;
	Suspect * suspect;
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
			suspect = it->second.suspect;

			//If the item exists and is in the same list
			if(suspect->needs_classification_update == false)
			{
				item->setText(str);
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
					item->setForeground(gbrush);
					item->setText(str);
					this->ui.benignList->addItem(item);
				}
				//If Hostile, old item is in benign
				else
				{
					//Remove old item, update info, change color, put in new list
					this->ui.benignList->removeItemWidget(item);
					item->setForeground(rbrush);
					item->setText(str);
					this->ui.hostileList->addItem(item);
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
					//Point to the new item
					it->second.item = item;
				}
				//If Hostile
				else
				{
					//Create the Suspect in list with info set alignment and color
					item = new QListWidgetItem(str, this->ui.hostileList, 0);
					item->setTextAlignment(Qt::AlignLeft|Qt::AlignBottom);
					item->setForeground(rbrush);
					//Point to the new item
					it->second.item = item;
				}
				//Reset the flags
				suspect->needs_feature_update = false;
				suspect->needs_classification_update = false;
			}
		}
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

void NovaGUI::on_SuspectButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(0);
}

void NovaGUI::on_CEButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(1);
}

void NovaGUI::on_DMButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(2);
}



void NovaGUI::on_HSButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(3);
}

void NovaGUI::on_LTMButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(4);
}

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

void closeNova()
{
	if(novaRunning)
	{
		//Sets the message
		data = "EXIT";
		dataLen = 4;

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
	// -------------------------------------------------------
	close(CE_OutSock);

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

void startNova()
{
	if(!novaRunning)
	{
		system("nohup honeyd -d -i eth0 -f Config/haystack.config -p $HOME/Programs/nmap-4.11/nmap-os-fingerprints -l "
				"Config/honeyd.log -s Config/honeydservice.log > /dev/null &");
		system("nohup honeyd -d -i lo -f Config/doppelganger.config -p $HOME/Programs/nmap-4.11/nmap-os-fingerprints -l "
				"Config/honeydDopp.log -s Config/honeydDoppservice.log 10.0.0.0/8 > /dev/null &");
		system("nohup ./bin/LocalTrafficMonitor -n Config/NOVAConfig.txt -l Config/Log4cxxConfig_LTM.xml > /dev/null &");
		system("nohup ./bin/Haystack -n Config/NOVAConfig.txt -l Config/Log4cxxConfig_HS.xml > /dev/null &");
		system("nohup ./bin/ClassificationEngine -n Config/NOVAConfig.txt -l Config/Log4cxxConfig_CE.xml > /dev/null &");
		system("nohup ./bin/DoppelgangerModule -n Config/NOVAConfig.txt -l Config/Log4cxxConfig_DM.xml > /dev/null &");
		novaRunning = true;
	}
}

void sclose(int sock)
{
	close(sock);
}
