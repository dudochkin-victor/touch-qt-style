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

#ifndef QTMEEGOFILEPICKER_H
#define QTMEEGOFILEPICKER_H

#include <QEventLoop>
#include <QStringList>

class SelectSingleContentItemPage;
class SelectMultipleContentItemsPage;
class MApplicationWindow;
class MApplicationPage;

class QtMeegoFilePicker : public QEventLoop
{
  Q_OBJECT
public:
    enum PickerMode {
        Mode_Unknown = -1,
        Mode_OpenFile = 0,
        Mode_OpenFiles = 1,
        Mode_SaveFile = 2,
        Mode_OpenDirectory = 3
    };

    QtMeegoFilePicker(QObject *parent = 0);
    virtual ~QtMeegoFilePicker();

    void setMode(PickerMode mode);
    QString getSelectedFileName();
    QStringList getSelectedFileNames();

    int exec(ProcessEventsFlags flags = QEventLoop::AllEvents);

public slots:
    void contentItemSelected( const QString &contentItem );
    void contentItemsSelected( const QStringList &contentItems );
    void cancel();

private:
    void openFile();
    void openFiles();
    MApplicationWindow* getApplicationWindow();

    PickerMode m_mode;
    QString m_selectedFileName;
    QStringList m_selectedFileNames;

    SelectSingleContentItemPage* m_singleItemPage;
    SelectMultipleContentItemsPage* m_multipleItemsPage;

    MApplicationWindow* m_window;
    MApplicationPage* m_dummyPage;
};

#endif
