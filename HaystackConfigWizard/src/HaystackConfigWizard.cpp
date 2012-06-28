//============================================================================
// Name        : HaystackConfigWizard.cpp
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

#include "HaystackConfigWizard.h"

//Custom Pages
#include "pages/WelcomePage.h"

using namespace std;

HaystackConfigWizard::HaystackConfigWizard(QWidget *parent) : QWizard(parent)
{
	setWindowTitle("Haystack Auto-Configuration Wizard");
    setWizardStyle(ModernStyle);
	addPage(new WelcomePage);
    setPixmap(QWizard::WatermarkPixmap, QPixmap("/usr/share/nova/icons/novaSplashBanner.png"));
    setPixmap(QWizard::LogoPixmap, QPixmap("/usr/share/nova/icons/nova-icon.png").scaledToHeight(50));
}
