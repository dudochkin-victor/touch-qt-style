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

#ifndef QTMAEMO6DIALOGDIMMER_H
#define QTMAEMO6DIALOGDIMMER_H

#include <QWidget>

class QPaintEvent;


/*!
 * This class takes care of dimming the background behind a dialog.
 */
class QtMaemo6DialogDimmer: public QWidget
{
    Q_OBJECT
public:
    explicit QtMaemo6DialogDimmer(QWidget *dialog);
    virtual ~QtMaemo6DialogDimmer();

protected:
    void paintEvent(QPaintEvent * event);

    bool event(QEvent *event);

Q_SIGNALS:
    void dimmerClicked();

private:
    QWidget *m_dialog;
};

#endif
