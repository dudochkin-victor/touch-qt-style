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

#ifndef QTMAEMO6APPLICATIONWINDOW_H
#define QTMAEMO6APPLICATIONWINDOW_H

#include "mapplicationwindow.h"
#include "mapplicationpage.h"
#include <QPointer>

class MHomeButtonPanel;
class MNavigationBar;
class MApplicationMenu;
class MLayout;
class MLinearLayoutPolicy;
class MStatusBar;

class QMainWindow;
class QtMaemo6Style;
class QtMaemo6ApplicationWindowEventFilter;

typedef WId Window;

class QtMaemo6ApplicationWindowEventFilter : public QObject {
    Q_OBJECT
public:
    QtMaemo6ApplicationWindowEventFilter(QObject* parent = 0) : QObject(parent) {}
    virtual bool eventFilter(QObject *obj, QEvent *event);
};



class QtMaemo6ApplicationWindow : public MApplicationWindow
{
    Q_OBJECT
private:
    //make it singleton
    QtMaemo6ApplicationWindow(QWidget *parent = NULL);
public:
    static QtMaemo6ApplicationWindow* createInstance(QWidget* parent);
    static QtMaemo6ApplicationWindow* instance();
    virtual ~QtMaemo6ApplicationWindow();
    
    void resizeEvent(QResizeEvent*e);

public Q_SLOTS:

    void showMenu();
    void hideMenu();
    void hideSubMenu();
    //void minimize();
    void showSubMenu();

Q_SIGNALS:
    void homeClicked();
    void escapeClicked();

protected:
    void showEvent(QShowEvent*e);
    void buildMenuTree();
    virtual bool eventFilter(QObject *, QEvent *);

protected Q_SLOTS:
    void showFullScreen();
    void showNavBarOnly();
    void doPageChange(MApplicationPage *page);
    void handleSubMenuAction();
    void disappearCurrentPage();
    void orientationChange(M::OrientationAngle);
    void reposition();

    virtual void mousePressEvent(QMouseEvent *event);
    
    bool event(QEvent* );

private:
    MHomeButtonPanel* homePanel;
    MEscapeButtonPanel* escapePanel;
    MNavigationBar* navBar;
    MStatusBar* statusBar;

    QMainWindow* m_mainWindow;

    //dummy page, used for the application menu
    QPointer<MApplicationPage> m_menuPage;
    //page used to show sub menus
    QPointer<MApplicationPage> m_subMenuPage;

    MLayout* m_subMenuPageLayout;
    MLinearLayoutPolicy* m_subMenuPageLayoutPolicy;

    MWindow* m_statusBarWindow;
    MWindow* m_navigationBarWindow;

    QtMaemo6ApplicationWindowEventFilter* m_windowEventFilter;

    //must use QWidget instead of a QSpacerItem, because the experimental client decoration patch
    // in Qt requires a QWidget pointer as parameter.
    QWidget* m_topDecorationSpacer;
    QWidget* m_bottomDecorationSpacer;
    QWidget* m_leftDecorationSpacer;
    QWidget* m_rightDecorationSpacer;
    
    static QtMaemo6ApplicationWindow* self;

    bool m_menuIsVisisble;

    QtMaemo6Style* m_style;
};

#endif // QTMAEMO6APPLICATIONWINDOW_H
