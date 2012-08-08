//============================================================================
// Name        : subnetPopup.h
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
// Description : Popup for creating and editing subnets
//============================================================================
#ifndef SUBNETPOPUP_H
#define SUBNETPOPUP_H

#include "ui_subnetPopup.h"
#include "HoneydConfiguration/HoneydConfigTypes.h"
#include "NovaUtil.h"
#include "HoneydConfiguration/HoneydConfiguration.h"

#include <QtGui/QSpinBox>
#include <arpa/inet.h>
#include <math.h>

class QRegExpValidator;

class MaskSpinBox : public QSpinBox
{
	Q_OBJECT

public:

	MaskSpinBox(QWidget * parent = 0, Nova::Subnet * s = NULL) : QSpinBox(parent)
	{
		this->lineEdit()->setReadOnly(true);
		setWrapping(true);
		//Num nodes + interface itself = num of addressable IP's required
		int n = s->m_nodes.size() +1;
		int count = 0;
		while((n/=2) > 0)
		{
			count++;
		}
		setRange(0, 32-count);
		setValue(s->m_maskBits);
	}

protected:
	int valueFromText(const QString &text) const
	{
		QString temp = QString(text);
		in_addr_t mask = ntohl(inet_addr(temp.toStdString().c_str()));
		return Nova::HoneydConfiguration::GetMaskBits(mask);
	}

	QString textFromValue(int value) const
	{
		in_addr_t mask = 0;
		int i;
		for(i = 0; i < value; i++)
		{
			mask++;
			mask = mask << 1;
		}
		mask = mask << (31-i);
		in_addr temp;
		temp.s_addr = ntohl(mask);
		return QString(inet_ntoa(temp));
	}
	QValidator::State validate(QString &text) const
	{
		if(this->hasFocus())
			return QValidator::Intermediate;
		else if((inet_addr(text.toStdString().c_str()) == INADDR_NONE))
			return QValidator::Invalid;
		return QValidator::Acceptable;
	}
};


class subnetPopup : public QMainWindow
{
    Q_OBJECT

public:

    subnetPopup(QWidget *parent = 0, Nova::Subnet *s  = NULL);
    ~subnetPopup();

    Nova::Subnet m_editSubnet;
    MaskSpinBox * m_maskEdit;

    //Saves the current configuration
    void SaveSubnet();
    //Loads the last saved configuration
    void LoadSubnet();
    //Copies the data from parent novaconfig and adjusts the pointers
    void PullData();
    //Copies the data to parent novaconfig and adjusts the pointers
    void PushData();


private Q_SLOTS:

	//General Button slots
	void on_okButton_clicked();
	void on_cancelButton_clicked();
	void on_restoreButton_clicked();
	void on_applyButton_clicked();

private:
    Ui::subnetPopupClass ui;
};

#endif // SUBNETPOPUP_H
