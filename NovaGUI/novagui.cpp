#include <sys/un.h>
#include <arpa/inet.h>
#include <QtGui>
#include <QApplication>
#include "novagui.h"
#include <sstream>
#include <QString>
#include <fstream>

using namespace std;
using namespace Nova;
using namespace ClassificationEngine;

int CEsock;
static SuspectHashTable SuspectTable;
//Might need locks later
pthread_rwlock_t lock;



NovaGUI::NovaGUI(QWidget *parent)
    : QMainWindow(parent)
{
	//Might need locks later
	pthread_rwlock_init(&lock, NULL);
	ui.setupUi(this);
	struct sockaddr_un CE_IPCAddress;
	int len;
	pthread_t CEListenThread;

	if((CEsock = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
			perror("socket");
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
			perror("bind");
			sclose(CEsock);
			exit(1);
	}

	if(listen(CEsock, 5) == -1)
	{
			perror("listen");
			sclose(CEsock);
			exit(1);
	}
	pthread_create(&CEListenThread,NULL,CEListen, this);
}

NovaGUI::~NovaGUI()
{

}

void NovaGUI::on_CEUpdate_clicked()
{
    //this->ReceiveCE(CEsock);
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
	        perror("accept");
			sclose(connectionSocket);
	        return false;
	    }
	    if((bytesRead = recv(connectionSocket, buf, MAX_MSG_SIZE, 0 )) == 0)
	    {
	    	return false;

	    }
	    else if(bytesRead == 1)
	    {
	        perror("recv");
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
			cout << suspect->ToString();
			// archive and stream closed when destructors are called
			updateSuspect(suspect);
		}
		catch(...)
		{
			cout << "Error interpreting received Suspect." <<endl;
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
	}
	pthread_rwlock_unlock(&lock);
}

void sclose(int sock)
{
	close(sock);
}


