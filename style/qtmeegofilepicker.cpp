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

#include "qtmeegofilepicker.h"

#include <QtGui/QFileDialog>
#include <QDebug>
#include <QApplication>
#include <QUrl>

#include <SelectSingleContentItemPage.h>
#include <SelectMultipleContentItemsPage.h>

#include <QtSparql/QSparqlConnection>
#include <QtSparql/QSparqlQuery>
#include <QtSparql/QSparqlResult>

#include "qtmaemo6applicationwindow.h"


static QString
trackerIdToFilename(const QString &trackerId)
{
    QSparqlQuery query( "SELECT ?u WHERE { ?:tUri nie:url ?u . }" );
    query.bindValue( "tUri", QUrl( trackerId ) ); //That puts <trackerId> into the query
    QSparqlConnection connection( "QTRACKER" ); //Exact string needed to make connection to tracker
    QSparqlResult *result = connection.exec( query );

    result->waitForFinished();
    result->first();

    QString resultFile;

    /* in case there are multiple items returned...
    while ( result->isValid() ) {
        // do things
        result->next();
    }
    */

    if ( result->isValid() ) {
        QUrl url ( result->binding (0).value().toString() );
        resultFile = url.toString();
    }

    delete result;

    return resultFile;
}

QtMeegoFilePicker::QtMeegoFilePicker(QObject * parent)
  : QEventLoop( parent ),
    m_mode( Mode_Unknown ),
    m_singleItemPage( 0 ),
    m_multipleItemsPage( 0 ),
    m_window(0),
    m_dummyPage(0)
{
}

QtMeegoFilePicker::~QtMeegoFilePicker()
{
    if(m_window)
        delete m_window;
}

int QtMeegoFilePicker::exec(ProcessEventsFlags flags)
{
    switch(m_mode)
    {
        case Mode_OpenFile:
            openFile();
            break;
        case Mode_OpenFiles:
            openFiles();
            break;
        //not implemented yet
        case Mode_OpenDirectory:
        case Mode_SaveFile:
        default:
            return 0;
    }
    return QEventLoop::exec(flags);
}

void QtMeegoFilePicker::openFile()
{
    MApplicationWindow* window = getApplicationWindow();

    m_singleItemPage = new SelectSingleContentItemPage("", QStringList(), "" );
    m_singleItemPage->setTitle("My Picker");
    connect(m_singleItemPage, SIGNAL( contentItemSelected( const QString & ) ),
            this, SLOT( contentItemSelected( const QString & ) ) );
    connect(m_singleItemPage, SIGNAL( backButtonClicked() ),
            this, SLOT( cancel() ) );


    m_singleItemPage->appear( window, MSceneWindow::DestroyWhenDismissed );
}

void QtMeegoFilePicker::openFiles()
{
    MApplicationWindow* window = getApplicationWindow();

    m_multipleItemsPage = new SelectMultipleContentItemsPage("", QStringList(), QStringList() );
    m_multipleItemsPage->setTitle("My Picker");
    connect(m_multipleItemsPage, SIGNAL( contentItemsSelected( const QStringList & ) ),
            this, SLOT( contentItemsSelected( const QStringList & ) ) );
    connect(m_multipleItemsPage, SIGNAL( contentItemsSelectionCancelled() ),
            this, SLOT( cancel() ) );
    connect(m_multipleItemsPage, SIGNAL( backButtonClicked() ),
            this, SLOT( cancel() ) );

    m_multipleItemsPage->appear( window, MSceneWindow::DestroyWhenDismissed );
}

void QtMeegoFilePicker::setMode(PickerMode mode)
{
    m_mode = mode;
}

QString QtMeegoFilePicker::getSelectedFileName()
{
    return m_selectedFileName;
}

QStringList QtMeegoFilePicker::getSelectedFileNames()
{
    return m_selectedFileNames;
}

void QtMeegoFilePicker::contentItemSelected( const QString &contentItem )
{
    m_selectedFileName = trackerIdToFilename( contentItem );

    m_singleItemPage->disappear();

    exit( 0 );
}

void QtMeegoFilePicker::contentItemsSelected( const QStringList &contentItems )
{
    m_selectedFileNames.clear();

    foreach( const QString& item, contentItems )
    {
        m_selectedFileNames << trackerIdToFilename( item );
    }

    m_multipleItemsPage->disappear();

    exit( 0 );
}

void QtMeegoFilePicker::cancel() {
    if(m_multipleItemsPage)
        m_multipleItemsPage->disappear();
    if(m_singleItemPage)
        m_singleItemPage->disappear();

    exit( 0 );
}

MApplicationWindow* QtMeegoFilePicker::getApplicationWindow() {
    if(!QtMaemo6ApplicationWindow::instance()) {
        if(!m_window) {
            QApplication* app = qobject_cast<QApplication*>(qApp);
            QWidget* window = 0;
            if(app)
                window = app->activeWindow();
            m_window = new MApplicationWindow(window);

            //this page is only needed, to make navigationbar and back button visible when file picker is shown
            m_dummyPage = new MApplicationPage();
            m_dummyPage->appear(m_window);
        }
        m_window->show();
        m_window->raise();
        return m_window;
    } else
        return QtMaemo6ApplicationWindow::instance();
}
