//============================================================================
// Name        : nova_manual.cpp
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

#include "nova_manual.h"

NovaGUI * mainw;

Nova_Manual::Nova_Manual(QWidget *parent)
    : QMainWindow(parent)
{
	ui.setupUi(this);
	mainw = (NovaGUI *)parent;

	paths = new QStringList();
	setPaths();

	// TODO: There's something up with the use of QStringList and
	//       passing it to setSourcePaths, as well as setSource.
	//		 both cause segfaults, Signal: SIGSEGV in the debugger,
	//		 and it says that it can't create the actual variable for paths...

	// set default help page here, as well as Text Browser Search Paths
	ui.displayHelp->setSearchPaths(*paths);
	ui.helpTree->setCurrentItem((ui.helpTree->itemAt(0,0)));
}

Nova_Manual::~Nova_Manual()
{

}

//Action to take when window is closing
void Nova_Manual::closeEvent(QCloseEvent * e)
{
	e = e;
	mainw->isHelpUp = false;
}

void Nova_Manual::setPaths()
{
	*paths << "/home/addison/Code/Nova/NovaGUI/HelpDocs/";
}

void Nova_Manual::on_helpTree_itemSelectionChanged()
{
	// Will have to find a more graceful way to do this, I don't know that
	// for the breadth of stuff that can populate the help that we'd want to
	// put in an if for each one. Maybe have an enumerated type for determining
	// keyword position in an array of strings for use in a switch statement?

	if(!(ui.helpTree->currentItem()->text(0)).compare(QString("Quickstart")))
	{
		ui.displayHelp->setSource(QUrl(QString("test.txt")));
	}
	else if(!(ui.helpTree->currentItem()->text(0)).compare(QString("Installing")))
	{
		ui.displayHelp->setSource(QUrl(QString("poem.txt")));
	}
}
