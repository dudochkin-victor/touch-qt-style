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
#include <QStyle>

MainWindow::MainWindow() : QMainWindow(NULL)
{
    setWindowTitle("Orientation Change Demo");
    
    if(!connect(style(), SIGNAL(orientationChanged(int)), this, SLOT(rotateImage(int))))
        qCritical() << "Could not connect to orientationChanged signal of the Qt Meegotouch style.  Maybe you are not running with the Maemo6 style?";
    m_landscapeImage.load(":/images/meegotouch-wallpaper-landscape.jpg");
    m_portraitImage.load(":/images/meegotouch-wallpaper-portrait.jpg");

    m_label = new QLabel(this);
    setCentralWidget(m_label);

    int orientation = 0;
    QMetaObject::invokeMethod(style(), "orientation", Q_RETURN_ARG(int, orientation));
    rotateImage(orientation);
}

MainWindow::~MainWindow()
{

}

void MainWindow::rotateImage(int orientationAngle) {
    QTransform t;
    t.rotate(orientationAngle);
    
    QPixmap pixmap;
    
    switch(orientationAngle) {
      case 0: 
        m_label->setAlignment(Qt::AlignTop);
        pixmap = m_landscapeImage;
        break;
      case 90: 
        m_label->setAlignment(Qt::AlignRight);
        pixmap = m_portraitImage;
        break;
      case 180: 
        m_label->setAlignment(Qt::AlignBottom);
        pixmap = m_landscapeImage;
        break;
      case 270: 
        m_label->setAlignment(Qt::AlignLeft);
        pixmap = m_portraitImage;
        break;
    }

    m_label->setPixmap(pixmap.transformed(t));
}
