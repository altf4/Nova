//============================================================================
// Name        : main.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Creates and runs the NovaGUI object
//============================================================================/*

#include "novagui.h"
#include "ui_novagui.h"
#include <QtGui>
#include <QApplication>
#include "main.h"

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
