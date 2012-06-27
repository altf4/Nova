//============================================================================
// Name        : WelcomePage.cpp
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
// Description : XXX TODO XXX
//============================================================================

#include "WelcomePage.h"
#include "../forms/InterfaceForm.h"

using namespace std;

QGridLayout * mainLayout;

WelcomePage::WelcomePage(QWidget *parent) : QWizardPage(parent)
{
	setTitle("Welcome to the Auto-Configuration Tool");
	setSubTitle("Please select which interfaces you wish to use.");
	QGridLayout *mainLayout = new QGridLayout();
	mainLayout->addWidget(new InterfaceForm(this));
	setLayout(mainLayout);
}
