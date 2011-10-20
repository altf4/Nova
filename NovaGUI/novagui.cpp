//============================================================================
// Name        : novagui.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : The main NovaGUI component, utilizes the auto-generated ui_novagui.h
//============================================================================/*

#include <sys/un.h>
#include <arpa/inet.h>
#include <QtGui>
#include <QApplication>
#include "novagui.h"
#include <sstream>
#include <QString>
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
//Might need locks later
pthread_rwlock_t lock;

LoggerPtr m_logger(Logger::getLogger("main"));



NovaGUI::NovaGUI(QWidget *parent)
    : QMainWindow(parent)
{
	//Might need locks later
	pthread_rwlock_init(&lock, NULL);
	ui.setupUi(this);
	struct sockaddr_un CE_IPCAddress;
	int len;
	pthread_t CEListenThread;
	DOMConfigurator::configure("Config/Log4cxxConfig_GUI.xml");


	if((CEsock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		LOG4CXX_ERROR(m_logger, "socket: " << strerror(errno));
		sclose(CEsock);
		exit(1);
	}

	CE_IPCAddress.sun_family = AF_UNIX;

	ofstream key(CE_FILENAME);
	key.close();
	strcpy(CE_IPCAddress.sun_path, CE_FILENAME);
	unlink(CE_IPCAddress.sun_path);
	len = strlen(CE_IPCAddress.sun_path) + sizeof(CE_IPCAddress.sun_family);

	if(bind(CEsock,(struct sockaddr *)&CE_IPCAddress,len) == -1)
	{
		LOG4CXX_ERROR(m_logger, "bind: " << strerror(errno));
		sclose(CEsock);
		exit(1);
	}

	if(listen(CEsock, 5) == -1)
	{
		LOG4CXX_ERROR(m_logger, "listen: " << strerror(errno));
		sclose(CEsock);
		exit(1);
	}
	pthread_create(&CEListenThread,NULL,CEListen, this);
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
	    else if(bytesRead == 1)
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
			updateSuspect(suspect);
		}
		catch(boost::archive::archive_exception e)
		{
			LOG4CXX_ERROR(m_logger, "Error interpreting received Suspect: " << string(e.what()));
		}
		return true;
}

void NovaGUI::updateSuspect(Suspect* suspect)
{
	if(suspect == NULL) return;

	pthread_rwlock_wrlock(&lock);
	//If the suspect already exists in our table
	if(SuspectTable.find(suspect->IP_address.s_addr) != SuspectTable.end())
	{
		//Update the information.
		delete SuspectTable[suspect->IP_address.s_addr];
		SuspectTable[suspect->IP_address.s_addr] = suspect;
	}
	//If the suspect is new
	else
	{
		//Put him in the table
		SuspectTable[suspect->IP_address.s_addr] = suspect;
	}
	pthread_rwlock_unlock(&lock);
	drawSuspects();
}

void NovaGUI::drawSuspects()
{
	//Create the colors for the draw
	QBrush gbrush(QColor(0, 150, 0, 255));
	gbrush.setStyle(Qt::NoBrush);
	QBrush rbrush(QColor(200, 0, 0, 255));
	rbrush.setStyle(Qt::NoBrush);
	QListWidgetItem *item;
	this->ui.benignList->clear();
	this->ui.hostileList->clear();
	QString str;

	pthread_rwlock_rdlock(&lock);
	for (SuspectHashTable::iterator it = SuspectTable.begin() ; it != SuspectTable.end(); it++)
	{
		if(it->second != NULL)
		{
			str = (QString)it->second->ToString().c_str();
			//If Benign
			if(it->second->classification == 0)
			{
				//Create a Separator
				new QListWidgetItem(this->ui.benignList);
				//Create the Suspect
				item = NULL;
				item = new QListWidgetItem(this->ui.benignList);
				item->setTextAlignment(Qt::AlignLeft|Qt::AlignBottom);
				item->setForeground(gbrush);
				item->setText(str);
			}
			//If Hostile
			else
			{
				//Create a Separator
				new QListWidgetItem(this->ui.hostileList);
				//Create the Suspect
				item = NULL;
				item = new QListWidgetItem(this->ui.hostileList);
				item->setTextAlignment(Qt::AlignLeft|Qt::AlignBottom);
				item->setForeground(rbrush);
				item->setText(str);
			}
		}
		else
		{
			LOG4CXX_INFO(m_logger, "NULL pointer in SuspectTable");
		}
	}
	pthread_rwlock_unlock(&lock);
}

void NovaGUI::on_CELoadButton_clicked()
{
	string line;
	string prefix;
	ifstream config("Config/NOVAConfig_CE.txt");

	if(config.is_open())
	{
		while(config.good())
		{
			getline(config,line);
			prefix = "INTERFACE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.CEInterfaceEdit->clear();
				this->ui.CEInterfaceEdit->appendPlainText((QString)line.c_str());
				continue;

			}
			prefix = "DATAFILE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.CEDatafileEdit->clear();
				this->ui.CEDatafileEdit->appendPlainText((QString)line.c_str());
				continue;
			}
			prefix = "IPC_MAX_CONNECTIONS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.CEIPCMaxConnectEdit->clear();
				this->ui.CEIPCMaxConnectEdit->appendPlainText((QString)line.c_str());
				continue;

			}
			prefix = "BROADCAST_ADDR";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.CEBroadcastAddrEdit->clear();
				this->ui.CEBroadcastAddrEdit->appendPlainText((QString)line.c_str());
				continue;

			}
			prefix = "SILENT_ALARM_PORT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.CESilentAlarmPortEdit->clear();
				this->ui.CESilentAlarmPortEdit->appendPlainText((QString)line.c_str());
				continue;
			}
			prefix = "K";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.CEKEdit->clear();
				this->ui.CEKEdit->appendPlainText((QString)line.c_str());
				continue;

			}
			prefix = "EPS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.CEEPSEdit->clear();
				this->ui.CEEPSEdit->appendPlainText((QString)line.c_str());
				continue;

			}
			prefix = "MAX_PTS";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.CEMaxPtsEdit->clear();
				this->ui.CEMaxPtsEdit->appendPlainText((QString)line.c_str());
				continue;
			}
			prefix = "CLASSIFICATION_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.CEClassificationTimeoutEdit->clear();
				this->ui.CEClassificationTimeoutEdit->appendPlainText((QString)line.c_str());
				continue;

			}
			prefix = "TRAINING_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.CETrainingTimeoutEdit->clear();
				this->ui.CETrainingTimeoutEdit->appendPlainText((QString)line.c_str());
				continue;

			}
			prefix = "MAX_FEATURE_VALUE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.CEMaxFeatureValEdit->clear();
				this->ui.CEMaxFeatureValEdit->appendPlainText((QString)line.c_str());
				continue;
			}
			prefix = "IS_TRAINING";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.CEEnableTrainingEdit->clear();
				this->ui.CEEnableTrainingEdit->appendPlainText((QString)line.c_str());
				continue;

			}
			prefix = "#";
			if(line.substr(0,prefix.size()).compare(prefix) && line.compare(""))
			{
				continue;
			}
		}
	}
	else
	{
		LOG4CXX_ERROR(m_logger, "Error loading from Classification Engine config file.");
		exit(1);
	}
	config.close();
}

void NovaGUI::on_CESaveButton_clicked()
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

}

void NovaGUI::on_DMLoadButton_clicked()
{
	string line;
	string prefix;
	ifstream config("Config/NOVAConfig_DM.txt");

	if(config.is_open())
	{
		while(config.good())
		{
			getline(config,line);
			prefix = "INTERFACE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.DMInterfaceEdit->clear();
				this->ui.DMInterfaceEdit->appendPlainText((QString)line.c_str());
				continue;

			}
			prefix = "DOPPELGANGER_IP";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.DMIPEdit->clear();
				this->ui.DMIPEdit->appendPlainText((QString)line.c_str());
				continue;
			}
			prefix = "ENABLED";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.DMEnabledEdit->clear();
				this->ui.DMEnabledEdit->appendPlainText((QString)line.c_str());
				continue;

			}
			prefix = "#";
			if(line.substr(0,prefix.size()).compare(prefix) && line.compare(""))
			{
				continue;
			}
		}
	}
	else
	{
		LOG4CXX_ERROR(m_logger, "Error loading from Doppelganger Module config file.");
		exit(1);
	}
	config.close();
}

void NovaGUI::on_DMSaveButton_clicked()
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

}

void NovaGUI::on_HSLoadButton_clicked()
{
	string line;
	string prefix;
	ifstream config("Config/NOVAConfig_HS.txt");

	if(config.is_open())
	{
		while(config.good())
		{
			getline(config,line);
			prefix = "INTERFACE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.HSInterfaceEdit->clear();
				this->ui.HSInterfaceEdit->appendPlainText((QString)line.c_str());
				continue;

			}
			prefix = "HONEYD_CONFIG";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.HSHoneyDConfigEdit->clear();
				this->ui.HSHoneyDConfigEdit->appendPlainText((QString)line.c_str());
				continue;

			}
			prefix = "TCP_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.HSTimeoutEdit->clear();
				this->ui.HSTimeoutEdit->appendPlainText((QString)line.c_str());
				continue;
			}
			prefix = "TCP_CHECK_FREQ";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.HSFreqEdit->clear();
				this->ui.HSFreqEdit->appendPlainText((QString)line.c_str());
				continue;

			}
			prefix = "#";
			if(line.substr(0,prefix.size()).compare(prefix) && line.compare(""))
			{
				continue;
			}
		}
	}
	else
	{
		LOG4CXX_ERROR(m_logger, "Error loading from Haystack Monitor config file.");
		exit(1);
	}
	config.close();
}

void NovaGUI::on_HSSaveButton_clicked()
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

}

void NovaGUI::on_LTMLoadButton_clicked()
{
	string line;
	string prefix;
	ifstream config("Config/NOVAConfig_LTM.txt");

	if(config.is_open())
	{
		while(config.good())
		{
			getline(config,line);
			prefix = "INTERFACE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.LTMInterfaceEdit->clear();
				this->ui.LTMInterfaceEdit->appendPlainText((QString)line.c_str());
				continue;

			}
			prefix = "TCP_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.LTMTimeoutEdit->clear();
				this->ui.LTMTimeoutEdit->appendPlainText((QString)line.c_str());
				continue;
			}
			prefix = "TCP_CHECK_FREQ";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				this->ui.LTMFreqEdit->clear();
				this->ui.LTMFreqEdit->appendPlainText((QString)line.c_str());
				continue;

			}
			prefix = "#";
			if(line.substr(0,prefix.size()).compare(prefix) && line.compare(""))
			{
				continue;
			}
		}
	}
	else
	{
		LOG4CXX_ERROR(m_logger, "Error loading from Local Traffic Monitor config file.");
		exit(1);
	}
	config.close();
}

void NovaGUI::on_LTMSaveButton_clicked()
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

}

void sclose(int sock)
{
	close(sock);
}
