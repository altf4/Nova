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
#include <sstream>
#include <QString>
#include <QChar>
#include <fstream>
#include <log4cxx/xml/domconfigurator.h>
#include <errno.h>

using namespace std;
using namespace Nova;
using namespace ClassificationEngine;
using namespace log4cxx;
using namespace log4cxx::xml;

int CEsock;
static SuspectHashTable SuspectTable;

pthread_rwlock_t lock;

LoggerPtr m_logger(Logger::getLogger("main"));



NovaGUI::NovaGUI(QWidget *parent)
    : QMainWindow(parent)
{
	//Might need locks later
	pthread_rwlock_init(&lock, NULL);
	ui.setupUi(this);
	DOMConfigurator::configure("Config/Log4cxxConfig_GUI.xml");
	//Not sure why this is needed, but it seems to take care of the error
	// the abstracted Qt operations of QObject::connect sometimes throws an
	// error complaining about queueing objects of type 'QItemSelection'
	qRegisterMetaType<QItemSelection>("QItemSelection");
	openSocket(this);
}

NovaGUI::~NovaGUI()
{

}


void *CEListen(void *ptr)
{
	while(true)
	{
		((NovaGUI*)ptr)->ReceiveCE(CEsock);
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
					this->ui.benignList->removeItemWidget(it->second.item);
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

/*void NovaGUI::on_CESaveButton_clicked()
{
	string line;
	ofstream config("Config/NOVAConfig_CE.txt");
	if(config.is_open())
	{
		config << "INTERFACE " << this->ui.CEInterfaceEdit->toPlainText().toStdString() << endl;
		config << "DATAFILE " << this->ui.CEDatafileEdit->toPlainText().toStdString() << endl;
		config << "IPC_MAX_CONNECTIONS " << this->ui.CEIPCMaxConnectEdit->toPlainText().toStdString() << endl;
		config << "BROADCAST_ADDR " << this->ui.CEBroadcastAddrEdit->toPlainText().toStdString() << endl;
		config << "SILENT_ALARM_PORT " << this->ui.CESilentAlarmPortEdit->toPlainText().toStdString() << endl;
		config << "K " << this->ui.CEKEdit->toPlainText().toStdString() << endl;
		config << "EPS " << this->ui.CEEPSEdit->toPlainText().toStdString() << endl;
		config << "MAX_PTS " << this->ui.CEMaxPtsEdit->toPlainText().toStdString() << endl;
		config << "CLASSIFICATION_TIMEOUT " << this->ui.CEClassificationTimeoutEdit->toPlainText().toStdString() << endl;
		config << "TRAINING_TIMEOUT " << this->ui.CETrainingTimeoutEdit->toPlainText().toStdString() << endl;
		config << "MAX_FEATURE_VALUE " << this->ui.CEMaxFeatureValEdit->toPlainText().toStdString() << endl;
		config << "IS_TRAINING " << this->ui.CEEnableTrainingEdit->toPlainText().toStdString();
	}
	else
	{
		LOG4CXX_ERROR(m_logger, "Error writing to Classification Engine config file.");
		exit(1);
	}
	config.close();
}*/

void NovaGUI::on_DMButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(2);
}


/*void NovaGUI::on_DMSaveButton_clicked()
{
	string line;
	ofstream config("Config/NOVAConfig_DM.txt");
	if(config.is_open())
	{
		config << "INTERFACE " << this->ui.DMInterfaceEdit->toPlainText().toStdString() << endl;
		config << "DOPPELGANGER_IP " << this->ui.DMIPEdit->toPlainText().toStdString() << endl;
		config << "ENABLED " << this->ui.DMEnabledEdit->toPlainText().toStdString();
	}
	else
	{
		LOG4CXX_ERROR(m_logger, "Error writing to Doppelganger Module config file.");
		exit(1);
	}
	config.close();

}*/

void NovaGUI::on_HSButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(3);
}

/*void NovaGUI::on_HSSaveButton_clicked()
{
	string line;
	ofstream config("Config/NOVAConfig_HS.txt");
	if(config.is_open())
	{
		config << "INTERFACE " << this->ui.HSInterfaceEdit->toPlainText().toStdString() << endl;
		config << "HONEYD_CONFIG " << this->ui.HSHoneyDConfigEdit->toPlainText().toStdString() << endl;
		config << "TCP_TIMEOUT " << this->ui.HSTimeoutEdit->toPlainText().toStdString() << endl;
		config << "TCP_CHECK_FREQ " << this->ui.HSFreqEdit->toPlainText().toStdString();
	}
	else
	{
		LOG4CXX_ERROR(m_logger, "Error writing to Haystack Monitor config file.");
		exit(1);
	}
	config.close();

}*/

void NovaGUI::on_LTMButton_clicked()
{
	this->ui.stackedWidget->setCurrentIndex(4);
}


/*void NovaGUI::on_LTMSaveButton_clicked()
{
	string line;
	ofstream config("Config/NOVAConfig_LTM.txt");
	if(config.is_open())
	{
		config << "INTERFACE " << this->ui.LTMInterfaceEdit->toPlainText().toStdString() << endl;
		config << "TCP_TIMEOUT " << this->ui.LTMTimeoutEdit->toPlainText().toStdString() << endl;
		config << "TCP_CHECK_FREQ " << this->ui.LTMFreqEdit->toPlainText().toStdString();
	}
	else
	{
		LOG4CXX_ERROR(m_logger, "Error writing to Local Traffic Monitor config file.");
		exit(1);
	}
	config.close();
}*/

void openSocket(NovaGUI *window)
{
	struct sockaddr_un CE_IPCAddress;
	int len;
	pthread_t CEListenThread;
	pthread_t CEDrawThread;


	if((CEsock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
		close(CEsock);
		exit(1);
	}

	CE_IPCAddress.sun_family = AF_UNIX;

	//Builds the key path
	string path = getenv("HOME");
	string key = CE_FILENAME;
	path += key;

	strcpy(CE_IPCAddress.sun_path, path.c_str());
	unlink(CE_IPCAddress.sun_path);
	len = strlen(CE_IPCAddress.sun_path) + sizeof(CE_IPCAddress.sun_family);

	if(bind(CEsock,(struct sockaddr *)&CE_IPCAddress,len) == -1)
	{
		LOG4CXX_ERROR(m_logger, "bind: " << strerror(errno));
		close(CEsock);
		exit(1);
	}

	if(listen(CEsock, SOCKET_QUEUE_SIZE) == -1)
	{
		LOG4CXX_ERROR(m_logger, "listen: " << strerror(errno));
		close(CEsock);
		exit(1);
	}
	pthread_create(&CEListenThread,NULL,CEListen, window);
	pthread_create(&CEDrawThread,NULL,CEDraw, window);
}

void sclose(int sock)
{
	close(sock);
}
