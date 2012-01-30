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
	ui.displayHelp->setSource(QUrl(QString("poem.txt")));
	//ui.helpTree->
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
	if((ui.helpTree->currentItem()->text(0)).compare(QString("Quickstart")))
	{
		ui.displayHelp->setSource(QUrl(QString("test.txt")));
	}
	else if((ui.helpTree->currentItem()->text(0)).compare(QString("Installing")))
	{
		ui.displayHelp->setSource(QUrl(QString("poem.txt")));
	}
}
