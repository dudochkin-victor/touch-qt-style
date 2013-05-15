/***************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (directui@nokia.com)
**
** This file is part of libmeegotouch.
**
** If you have questions regarding the use of this file, please contact
** Nokia at directui@nokia.com.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/

#include "mainwindow.h"

#include <QMenu>
#include <QFileDialog>
#include <QDebug>

MainWindow::MainWindow() : QMainWindow(NULL)
{
    setupUi(this);
    QMenu *menu(menuBar()->addMenu("Long menu"));

    for (int i = 0; i < 200; ++i) {
        menu->addAction(QString("Entry %1").arg(i));
        comboBox->addItem(QString("%1 cm").arg(i * 2));
    }
    connect(actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(actionOpenFiles, SIGNAL(triggered()), this, SLOT(openFiles()));
    connect(actionFullscreen, SIGNAL(triggered(bool)), SLOT(toogleFullscreen(bool)));

    connect(actionQuit, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(quitButton, SIGNAL(clicked()), qApp, SLOT(quit()));
    if(!connect(style(), SIGNAL(orientationChanged(int)), this, SLOT(setStatusBarText())))
        qCritical() << "Couldn't connect statusbar";

    statusBarLabel = new QLabel();
    statusBar()->addWidget(statusBarLabel);

    statusBarLabel->setText("I am the status bar");
}

MainWindow::~MainWindow()
{

}

void MainWindow::openFile()
{
    QString s = QFileDialog::getOpenFileName();

    statusBarLabel->setText( "open file: " + s );
}

void MainWindow::openFiles()
{
    QStringList sl = QFileDialog::getOpenFileNames();

    statusBarLabel->setText( "open files: " + sl.join( " " ) );
}

void MainWindow::setStatusBarText() {
    int retVal;
    if(QMetaObject::invokeMethod(qApp->style(), "orientation", Q_ARG(int*, &retVal)))
        statusBarLabel->setText(QString("Orientation %1�").arg(retVal));
    else
        statusBarLabel->setText("Couldn't retrive orientation");
}

void MainWindow::toogleFullscreen(bool fullscreen) {
    if(!fullscreen)
        setWindowState(windowState() ^ Qt::WindowFullScreen);
    else
        setWindowState(windowState() | Qt::WindowFullScreen);
}
