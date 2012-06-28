#include <QCheckBox>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sstream>
#include <errno.h>

#include "Logger.h"
#include "Config.h"
#include "ipRangeSelectionForm.h"

using namespace std;
using namespace Nova;

ipRangeSelectionForm::ipRangeSelectionForm(QWidget *parent)
    : QWidget(parent)
{
	ui.setupUi(this);
	struct ifaddrs * devices = NULL;
	struct ifaddrs *curIf = NULL;
	stringstream ss;
	MaskSpinBox *maskBox = new MaskSpinBox(this);
	ui.bitmaskHLayout->insertWidget(1, maskBox);
	maskBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	m_interfaceCheckBoxes = new QButtonGroup(ui.interfaceGroupBox);
	//Get a list of interfaces
	if(getifaddrs(&devices))
	{
		LOG(ERROR, string("Ethernet Interface Auto-Detection failed: ") + string(strerror(errno)), "");
	}
	for(curIf = devices; curIf != NULL; curIf = curIf->ifa_next)
	{
		if(((int)curIf->ifa_addr->sa_family == AF_INET) && !(curIf->ifa_flags & IFF_LOOPBACK))
		{
			QCheckBox * checkBox = new QCheckBox(QString(curIf->ifa_name), ui.interfaceGroupBox);
			checkBox->setObjectName(QString(curIf->ifa_name));
			checkBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
			m_interfaceCheckBoxes->addButton(checkBox);
			ui.interfaceGroupBoxGridLayout->addWidget(checkBox);
		}
	}
	freeifaddrs(devices);

	vector<string> ifList = Config::Inst()->GetInterfaces();
	while(!ifList.empty())
	{
		QCheckBox* checkObj = ui.interfaceGroupBox->findChild<QCheckBox *>(QString::fromStdString(ifList.back()));
		if(checkObj != NULL)
		{
			checkObj->setChecked(true);
		}
		ifList.pop_back();
	}
}

ipRangeSelectionForm::~ipRangeSelectionForm()
{

}
