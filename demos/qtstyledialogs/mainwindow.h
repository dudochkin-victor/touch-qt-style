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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "ui_mainwindow.h"
#include "ui_bookflight.h"
#include "ui_fullscreen_non_modal.h"
#include "ui_fullscreen_modal.h"

class MainWindow : public QMainWindow, Ui::MainWindow
{
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow();
public slots:
    void showNormalNmDialog();
    void showFullScreenNmDialog();
    void showContentNmDialog();

    void showNormalMDialog();
    void showFullScreenMDialog();
    void showContentMDialog();

    void showErrorMessage();
    void showFileDialog();
    void showFontDialog();
    void showInputDialog();
    void showMessageBox();
    void showProgressDialog();
    void showWizard();

private:
    void createAirportBox(QComboBox *box);
    Ui::BookFlightUi uiBookFlight;
    Ui::FullscreenNonModalUi uiFullscreenNonModal;
    Ui::FullscreenModalUi uiFullscreenModal;
};

#endif
