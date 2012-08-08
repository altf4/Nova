//============================================================================
// Name        : nodePopup.h
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
// Description : Popup for creating and editing nodes
//============================================================================
#ifndef NODEPOPUP_H
#define NODEPOPUP_H

#include "ui_nodePopup.h"
#include "HoneydConfiguration/HoneydConfigTypes.h"
#include "DialogPrompter.h"

#include <QtGui/QSpinBox>
#include <math.h>
#include <string.h>

enum macType{macPrefix = 0, macSuffix = 1};

class QRegExpValidator;

class HexMACSpinBox : public QSpinBox
{
	Q_OBJECT

public:
	HexMACSpinBox(QWidget *parent = 0, std::string MACAddr = "", macType which = macSuffix) : QSpinBox(parent)
	{
		m_validator = new QRegExpValidator(QRegExp("([0-9A-Fa-f][0-9A-Fa-f][:]){0,2}[0-9A-Fa-f][0-9A-Fa-f]"
				, Qt::CaseInsensitive, QRegExp::RegExp), this);
		setCorrectionMode(QAbstractSpinBox::CorrectToNearestValue);
		setWrapping(true);
		setRange(0, (::pow(2,24)-1));

		if(MACAddr.size() < 17)
		{
			setValue(0);
		}

		else if(which == macSuffix)
		{
			QString suffixStr = QString(MACAddr.substr(9, MACAddr.size()).c_str()).toLower();
			suffixStr = suffixStr.remove(':');
			setValue(suffixStr.toInt(NULL, 16));
		}
		else
		{
			QString prefixStr = QString(MACAddr.substr(0, 8).c_str()).toLower();
			prefixStr = prefixStr.remove(':');
			setValue(prefixStr.toInt(NULL, 16));
		}
	}

protected:
	int valueFromText(const QString &text) const
	{
		int val = 0;
		QString temp = QString(text);
		temp = temp.remove(':');
		temp = temp.right(6);
		val = temp.toInt(NULL, 16);
		return val;
	}

	QString textFromValue(int value) const
	{
		QString ret = QString::number(value, 16).toLower();
		while(ret.toStdString().size() < 6)
		{
			ret = ret.prepend('0');
		}
		ret = ret.insert(4, ':');
		ret = ret.insert(2, ':');
		return ret;
	}

private:

    QRegExpValidator *m_validator;
};


class nodePopup : public QMainWindow
{
    Q_OBJECT

public:

    DialogPrompter *m_prompter;

    nodePopup(QWidget *parent = 0, Nova::Node *n  = NULL, bool editingNode = false);
    ~nodePopup();

    HexMACSpinBox *m_ethernetEdit;
    HexMACSpinBox *m_prefixEthEdit;

    //Saves the current configuration
    void SaveNode();

    //Loads the last saved configuration
    void LoadNode();

    //Checks for IP or MAC conflicts
    Nova::nodeConflictType ValidateNodeSettings();

private Q_SLOTS:

	//General Button slots
	void on_okButton_clicked();
	void on_cancelButton_clicked();
	void on_restoreButton_clicked();
	void on_applyButton_clicked();
	void on_generateButton_clicked();
	void on_isDHCP_stateChanged();
	void on_isRandomMAC_stateChanged();

private:
	Nova::Node *m_parentNode;
	bool m_editingNode;
    Ui::nodePopupClass ui;
    Nova::Node m_editNode;

};

#endif // NODEPOPUP_H
