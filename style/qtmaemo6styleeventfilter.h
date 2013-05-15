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

#ifndef QTMAEMO6STYLEEVENTFILTER_H
#define QTMAEMO6STYLEEVENTFILTER_H

#include "qtmaemo6style.h"

/*!
 * the eventfilter for the basic style. Currently this is only used for
 * testing purposes and may be deprecated
 */
class QtMaemo6StyleEventFilter : public QObject
{
    Q_OBJECT
public:
    explicit QtMaemo6StyleEventFilter(QtMaemo6Style *parent);
    virtual ~QtMaemo6StyleEventFilter() {};
protected:
    /*! \reimp */
    bool eventFilter(QObject *obj, QEvent *event);
    /*! \reimp_end */

protected:
    QtMaemo6Style *m_style;
    static QStringList nonScrollableWidgets;

protected Q_SLOTS:
    void dialogDestroyed();

};


#endif //QTMAEMO6STYLEEVENTFILTER_H
