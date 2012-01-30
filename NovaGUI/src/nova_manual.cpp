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
