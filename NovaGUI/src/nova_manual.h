//============================================================================
// Name        : nova_manual.h
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
// Description : "Help" dialog to be displayed in the GUI
//============================================================================

#ifndef NOVA_MANUAL_H
#define NOVA_MANUAL_H

#include <QtGui/QMainWindow>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStringList>
#include <QString>
#include "ui_nova_manual.h"
#include "novagui.h"

class Nova_Manual : public QMainWindow
{
    Q_OBJECT

public:
    Nova_Manual(QWidget *parent);
    ~Nova_Manual();
    void closeEvent(QCloseEvent * e);
    void setToSelected();
    void setPaths();

private:
    Ui::Nova_ManualClass ui;
    QStringList *paths;

private slots:
	void on_helpTree_itemSelectionChanged();
};

#endif // NOVA_MANUAL_H
