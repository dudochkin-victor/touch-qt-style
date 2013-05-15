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

#include <QApplication>
#include <QPushButton>
#include <QErrorMessage>
#include <QFileDialog>
#include <QFontDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QWizard>
#include <QStringList>
#include <QDebug>

MainWindow::MainWindow() : QMainWindow(NULL)
{
    setupUi(this);

    connect(actionQuit, SIGNAL(triggered()), qApp, SLOT(quit()));

    connect(normalNmDialogButton, SIGNAL(clicked()), SLOT(showNormalNmDialog()));
    connect(fullScreenNmDialogButton, SIGNAL(clicked()), SLOT(showFullScreenNmDialog()));
    connect(contentNmDialogButton, SIGNAL(clicked()), SLOT(showContentNmDialog()));

    connect(normalMDialogButton, SIGNAL(clicked()), SLOT(showNormalMDialog()));
    connect(fullScreenMDialogButton, SIGNAL(clicked()), SLOT(showFullScreenMDialog()));
    connect(contentMDialogButton, SIGNAL(clicked()), SLOT(showContentMDialog()));

    connect(errorMessageButton, SIGNAL(clicked()), SLOT(showErrorMessage()));
    connect(fileDialogButton, SIGNAL(clicked()), SLOT(showFileDialog()));
    connect(fontDialogButton, SIGNAL(clicked()), SLOT(showFontDialog()));
    connect(inputDialogButton, SIGNAL(clicked()), SLOT(showInputDialog()));
    connect(messageBoxButton, SIGNAL(clicked()), SLOT(showMessageBox()));
    connect(progressDialogButton, SIGNAL(clicked()), SLOT(showProgressDialog()));
    connect(wizardButton, SIGNAL(clicked()), SLOT(showWizard()));
}

MainWindow::~MainWindow()
{

}

void MainWindow::showErrorMessage()
{
    QErrorMessage dialog(this);
    dialog.showMessage(tr("Errare humanum est, sed in errare perseverare diabolicum."));
    dialog.exec();
}

void MainWindow::showFileDialog()
{
    QString testFileName = QFileDialog::getOpenFileName(this,
                           tr("Please choose a file name for testing"),
                           "/");
}

void MainWindow::showFontDialog()
{
    bool ok = false;
    QFont testFont = QFontDialog::getFont(&ok, QFont("Times", 12),
                                          this,
                                          tr("Please choose a font for testing."));
}

void MainWindow::showInputDialog()
{
    QString testText = QInputDialog::getText(this,
                       tr("Please enter a string for testing."),
                       tr("This is a test"));
}

void MainWindow::showMessageBox()
{
    QMessageBox::warning(this,
                         tr("Don't Panic! This is a test!"),
                         tr("Error 422001: I'm sorry. I'm afraid I can't do that!"));
}

void MainWindow::showProgressDialog()
{
    QProgressDialog dialog(this);
    dialog.setValue(40);
    dialog.exec();
}

void MainWindow::showWizard()
{
    QWizard dialog(this);
    dialog.exec();
}

void MainWindow::showNormalNmDialog()
{
    QDialog *dialog = new QDialog(this);
    dialog->setAttribute( Qt::WA_DeleteOnClose, true);
    dialog->show();
}

void MainWindow::showFullScreenNmDialog()
{
    QDialog *dialog = new QDialog(this);
    dialog->setAttribute( Qt::WA_DeleteOnClose, true);
    uiFullscreenNonModal.setupUi(dialog);
    connect( uiFullscreenNonModal.closeButton, SIGNAL(clicked()), dialog, SLOT(accept()));
    dialog->showFullScreen();
}

void MainWindow::showContentNmDialog()
{
    QDialog *dialog = new QDialog(this);
    dialog->setAttribute( Qt::WA_DeleteOnClose, true);
    uiBookFlight.setupUi(dialog);
    createAirportBox(uiBookFlight.flyFromComboBox);
    createAirportBox(uiBookFlight.flyToComboBox);
    dialog->show();
}

void MainWindow::showNormalMDialog()
{
    QDialog dialog(this);
    dialog.exec();
}

void MainWindow::showFullScreenMDialog()
{
    QDialog dialog(this);
    uiFullscreenModal.setupUi(&dialog);
    connect( uiFullscreenModal.closeButton, SIGNAL(clicked()), &dialog, SLOT(accept()));
    dialog.showFullScreen();
    dialog.exec();
}

void MainWindow::showContentMDialog()
{
    QDialog dialog(this);
    uiBookFlight.setupUi(&dialog);
    createAirportBox(uiBookFlight.flyFromComboBox);
    createAirportBox(uiBookFlight.flyToComboBox);
    dialog.exec();
}

void MainWindow::createAirportBox(QComboBox *box)
{
    QStringList airports;
    airports << tr("Shire (SHI)")
             << tr("Gondor (GND)")
             << tr("Rohan (ROA)")
             << tr("Mordor International (MIA)")
             << tr("Mordor Barad-Dur (MBD)")
             << tr("Mordor Nurn (MNU)")
             << tr("Rivendell (RIV)")
             << tr("Mirkwood (MWI)")
             << tr("Trollshaws (TRO)")
             << tr("Lothlorien (LOT)");
    airports.sort();
    box->addItems(airports);
}
