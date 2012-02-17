//============================================================================
// Name        : main.cpp
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
// Description : Creates and runs the NovaGUI object
//============================================================================
#include "main.h"
#include "novagui.h"
#include "ui_novagui.h"

#include <QApplication>
#include <QPlastiqueStyle>

using namespace std;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(new QPlastiqueStyle());
    QFont novaFont = QFont("Arial", 10, QFont::Normal, false);
    QPalette novaPalette = a.style()->standardPalette();
    novaPalette.setBrush(QPalette::ToolTipBase, QBrush(QColor(255,255,255)));
    novaPalette.setBrush(QPalette::ToolTipText, QBrush(QColor(0,0,0)));
    a.setFont(novaFont);
    a.setPalette(novaPalette);

    NovaGUI w;
    w.show();
    a.exec();
}
