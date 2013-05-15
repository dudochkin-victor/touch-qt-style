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

#include "qtmaemo6dialogdimmer.h"

#include <QPaintEvent>
#include <QPainter>
#include <QApplication>

#include "mdeviceprofile.h"

#define DIM_ALPHA 200
#define DIM_GREY    0


QtMaemo6DialogDimmer::QtMaemo6DialogDimmer(QWidget *dialog)
    : QWidget((QWidget *) dialog->parent(), Qt::Dialog)
    , m_dialog(dialog)
{
    // // Grab screenshot of the active window (the main window)
    // QPixmap screenshot = QPixmap::grabWidget(qApp->activeWindow());
    // // screenshot.save("/tmp/dimmer_screenshot_raw.jpg");

    // // Dim screenshot
    // QImage dimmed(screenshot.size(), QImage::Format_ARGB32);
    // dimmed.fill(qRgba(DIM_GREY, DIM_GREY, DIM_GREY, DIM_ALPHA));

    // QPainter painter(&dimmed);
    // painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    // // painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    // painter.drawPixmap(0, 0, screenshot);
    // painter.end();

    // // dimmed.save("/tmp/dimmer_screenshot_dimmed.jpg");

    // // Use dimmed screenshot as background pixmap
    // // QPalette pal = palette();
    // // pal.setBrush(QPalette::Window, QPixmap::fromImage(dimmed));
    // // setPalette(pal);
    // // setBackgroundRole(QPalette::Window);
    // // setAutoFillBackground(true);
    // // resize(screenshot.size());

    setFixedSize(MDeviceProfile::instance()->resolution());

    setAttribute( Qt::WA_TranslucentBackground, true );
}

QtMaemo6DialogDimmer::~QtMaemo6DialogDimmer()
{
}

void QtMaemo6DialogDimmer::paintEvent(QPaintEvent * event)
{
    if (event) {
        QPainter painter(this);
        painter.fillRect(event->rect(), QColor( DIM_GREY, DIM_GREY, DIM_GREY, DIM_ALPHA ) );
    }
}

bool QtMaemo6DialogDimmer::event(QEvent *event)
{
    switch( event->type() ) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
        {
            emit dimmerClicked();
            return true;
        }
        default:
            return QWidget::event( event );
    }
}
