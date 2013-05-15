#include "qtmaemo6applicationwindow.h"
#include "qtmaemo6style_p.h"
#include "mhomebuttonpanel.h"
#include "mnavigationbar.h"
#include "mescapebuttonpanel.h"
#include "mapplicationmenu.h"
#include "mcomponentdata.h"
#include "mondisplaychangeevent.h"
#include "maction.h"
#include "mdeviceprofile.h"
#include "mnavigationbarstyle.h"
#include "mstatusbarstyle.h"
#include "mlayout.h"
#include "mlinearlayoutpolicy.h"
#include "mbutton.h"
#include "mstatusbar.h"
#include "mscene.h"
#include <MDebug>

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QDebug>
#include <QSpacerItem>
#include <QPushButton>
#include <QPointer>
#include <QLayout>
#include <QX11Info>
#ifdef Q_WS_X11
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif
//krazy:excludeall=qmethods

#ifdef HAVE_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif //HAVE_XDAMAGE

extern void qt_setMainWindowTitleWidget(QMainWindow *mainWindow, Qt::DockWidgetArea area, QWidget *widget);

void setWindowVisibility(MWindow* window, bool visible)
{
    MOnDisplayChangeEvent* ev = new MOnDisplayChangeEvent(visible, QRectF(QPointF(0, 0), window->visibleSceneSize()));
    QApplication::instance()->sendEvent(window, ev);
}


void setWindowVisibility(Window window, bool visible)
{
    Q_FOREACH(MWindow * win, MComponentData::instance()->windows()) {
        if (win && win->effectiveWinId() == window) {
            setWindowVisibility(win, visible);
        }
    }
}


bool my_x11Event(void *message, long int *result)
{
    Q_UNUSED(result)
    XEvent* event = static_cast<XEvent*>(message);

#ifdef HAVE_XDAMAGE
    int xDamageEventBase = 0;
    int xDamageErrorBase = 0;

    XDamageQueryExtension(QX11Info::display(), &xDamageEventBase, &xDamageErrorBase);

    if (event->type == xDamageEventBase + XDamageNotify) {
        QEvent* event = new QEvent(QEvent::UpdateRequest);
        QApplication::sendEvent(QtMaemo6ApplicationWindow::instance(), event);
        return true;
    }
#endif //HAVE_XDAMAGE
/*
    if (event->type == VisibilityNotify) {
        qCritical() << "XWindow Visibility";
        XVisibilityEvent *xevent = (XVisibilityEvent *) event;

        switch (xevent->state) {
        case VisibilityFullyObscured:
            setWindowVisibility(xevent->window, false);
            break;
        case VisibilityUnobscured:
        case VisibilityPartiallyObscured:
            setWindowVisibility(xevent->window, true);
            break;
        default:
            break;
        }
    }
*/
    return false;
}


bool QtMaemo6ApplicationWindowEventFilter::eventFilter(QObject *obj, QEvent *event)
{   //only do this for MWindows
    if(MWindow* window = qobject_cast<MWindow*>(obj)) {
        switch(event->type()) {
        case QEvent::Show:
            setWindowVisibility(window, true);
            break;
        case QEvent::Hide:
            setWindowVisibility(window, false);
            break;
        default:
            break;
        }
    }
    return false;
}

QtMaemo6ApplicationWindow* QtMaemo6ApplicationWindow::self = NULL;

QtMaemo6ApplicationWindow::QtMaemo6ApplicationWindow(QWidget *parent)
    : MApplicationWindow(parent)
    , homePanel(0)
    , escapePanel(0)
    , navBar(0)
    , statusBar(0)
    , m_mainWindow(qobject_cast<QMainWindow*>(parent))
    , m_menuPage(0)
    , m_subMenuPage(0)
    , m_subMenuPageLayout(0)
    , m_subMenuPageLayoutPolicy(0)
    , m_statusBarWindow(0)
    , m_navigationBarWindow(0)
    , m_windowEventFilter(new QtMaemo6ApplicationWindowEventFilter())
    , m_topDecorationSpacer(0)
    , m_bottomDecorationSpacer(0)
    , m_leftDecorationSpacer(0)
    , m_rightDecorationSpacer(0)
    , m_menuIsVisisble(false)
    , m_style(qobject_cast<QtMaemo6Style*>(qApp->style()))
{
    setObjectName("FullScreenWindow");
    //set FramelessWindowHint to Mainwindow to suppress MDecorator
    m_mainWindow->setWindowFlags(m_mainWindow->windowFlags() | Qt::FramelessWindowHint);
    installEventFilter(m_windowEventFilter);

    QSize screenSize = MDeviceProfile::instance()->resolution();

    //add spacer to the main window
    m_topDecorationSpacer = new QWidget();
    m_topDecorationSpacer->setFixedWidth(screenSize.width());
    m_bottomDecorationSpacer = new QWidget();
    m_bottomDecorationSpacer->setFixedWidth(screenSize.width());
    m_leftDecorationSpacer = new QWidget();
    m_leftDecorationSpacer->setFixedWidth(screenSize.height());
    m_rightDecorationSpacer = new QWidget();
    m_rightDecorationSpacer->setFixedWidth(screenSize.height());
    qt_setMainWindowTitleWidget(m_mainWindow, Qt::TopDockWidgetArea, m_topDecorationSpacer);
    qt_setMainWindowTitleWidget(m_mainWindow, Qt::BottomDockWidgetArea, m_bottomDecorationSpacer);
    qt_setMainWindowTitleWidget(m_mainWindow, Qt::LeftDockWidgetArea, m_leftDecorationSpacer);
    qt_setMainWindowTitleWidget(m_mainWindow, Qt::RightDockWidgetArea, m_rightDecorationSpacer);

    setBackgroundBrush(Qt::transparent);
    setViewportUpdateMode(BoundingRectViewportUpdate);
    setCacheMode(QGraphicsView::CacheBackground);

    m_menuPage = new MApplicationPage();
    m_menuPage->setObjectName("QtTransparentApplicationPage");
    m_menuPage->setTitle(m_mainWindow->windowTitle());
    m_menuPage->appear(this);

    m_subMenuPage = new MApplicationPage();
    m_subMenuPageLayout = new MLayout(m_subMenuPage->centralWidget());
    m_subMenuPageLayoutPolicy = new MLinearLayoutPolicy(m_subMenuPageLayout, Qt::Vertical);
    m_subMenuPageLayoutPolicy->setSpacing(0);
    m_subMenuPageLayoutPolicy->setContentsMargins(0,0,0,0);
    m_subMenuPageLayout->setLandscapePolicy(m_subMenuPageLayoutPolicy);

    foreach(QGraphicsItem* item, items()) {
        if(!homePanel) {
            homePanel = dynamic_cast<MHomeButtonPanel*>(item);
            if(homePanel)
                continue;
        }
        if(!escapePanel) {
            escapePanel = dynamic_cast<MEscapeButtonPanel*>(item);
            if(escapePanel)
                continue;
        }
        if(!navBar) {
            navBar = dynamic_cast<MNavigationBar*>(item);
            if(navBar)
                continue;
        }
        if(!statusBar) {
            statusBar = dynamic_cast<MStatusBar*>(item);
            if(statusBar)
                continue;
        }
    }

    if(!homePanel || !escapePanel || !navBar || !statusBar) {
        mDebug("PlainQt Style") << "Meego elements not found:\n homePanel" << homePanel << "\nescapePanel" << escapePanel << "\nnavBar" << navBar << "\nstatusBar" << statusBar;
    }
    statusBar->setManagedManually(true);

    QPalette pal = palette();
    pal.setBrush(backgroundRole(), Qt::transparent);
    setPalette(pal);
    QPalette pal2 = viewport()->palette();
    pal2.setBrush(viewport()->backgroundRole(), Qt::transparent);
    viewport()->setPalette(pal2);
    
    viewport()->setAutoFillBackground(false);
    setAutoFillBackground(false);

    viewport()->setAttribute(Qt::WA_OpaquePaintEvent,false);
    setAttribute(Qt::WA_OpaquePaintEvent,false);

    //qApp->setEventFilter(my_x11Event);
    m_mainWindow->installEventFilter(this);
    
    if(m_mainWindow->centralWidget())
        m_mainWindow->centralWidget()->setAttribute(Qt::WA_NoSystemBackground);
    
    m_navigationBarWindow = new MWindow(sceneManager(), parent);
    m_navigationBarWindow->setObjectName("NavigationBarWindow");
    m_navigationBarWindow->installEventFilter(m_windowEventFilter);

    m_statusBarWindow = new MWindow(sceneManager(), parent);
    m_statusBarWindow->setObjectName("StatusBarWindow");
    m_statusBarWindow->installEventFilter(m_windowEventFilter);

    buildMenuTree();

    if(m_style) {
        //connect(this, SIGNAL(orientationChanged(M::OrientationAngle)), SLOT(reposition()));
        if(!connect(m_style, SIGNAL(orientationChanged(M::OrientationAngle)), SLOT(reposition())))
            qCritical() << "Could not connect to orientationChanged signal";
        if(!m_style->isOrientationChangeEnabled()) {
            setOrientationAngle(M::Angle0);
            setOrientationLocked(true);
        } 
    }

    //disconnect the menu stuff, to turn off all libmeego automatics!
    // We have to switch visibility of our windows, before anything is triggered there
    navBar->disconnect();

    connect(navBar, SIGNAL(viewmenuTriggered()), SLOT(showMenu()));
    connect(navBar, SIGNAL(backButtonClicked()), SLOT(disappearCurrentPage()));
    connect(navBar, SIGNAL(closeButtonClicked()), parent, SLOT(close()));
    connect(navBar, SIGNAL(disappeared()), this, SLOT(debugSlot()));
    
    connect(homePanel, SIGNAL(buttonClicked()), SIGNAL(homeClicked()));
    connect(homePanel, SIGNAL(buttonClicked()), parent, SLOT(showMinimized()));

    connect(this, SIGNAL(pageChanged(MApplicationPage*)), this, SLOT(doPageChange(MApplicationPage*)));

    //this window is for the fullscreen view, we hide it by default
    reposition();
}

QtMaemo6ApplicationWindow* QtMaemo6ApplicationWindow::createInstance(QWidget* parent) {
    if(!self)
        self = new QtMaemo6ApplicationWindow(parent);
    return self;
}

QtMaemo6ApplicationWindow* QtMaemo6ApplicationWindow::instance() {
    return self;
}

QtMaemo6ApplicationWindow::~QtMaemo6ApplicationWindow() {
    self = NULL;
}

bool QtMaemo6ApplicationWindow::event(QEvent* event) {
    if(event->type() == QEvent::UpdateRequest) {
        statusBar->update();
    }
    return MApplicationWindow::event(event);
}

void QtMaemo6ApplicationWindow::resizeEvent(QResizeEvent* e)
{
    MApplicationWindow::resizeEvent(e);
    reposition();
}

void QtMaemo6ApplicationWindow::showEvent(QShowEvent* e)
{
    MApplicationWindow::showEvent(e);
} 

void QtMaemo6ApplicationWindow::doPageChange(MApplicationPage *page) {
    if(page != m_menuPage)
        showFullScreen(); // krazy:exclude=qmethods
    else
        showNavBarOnly();
}

void QtMaemo6ApplicationWindow::showFullScreen() {
    show();
    raise();
}

void QtMaemo6ApplicationWindow::showNavBarOnly() {
    if(!m_mainWindow->windowState().testFlag(Qt::WindowFullScreen)) {
        m_statusBarWindow->show();
        m_statusBarWindow->raise();
        m_navigationBarWindow->show();
        m_navigationBarWindow->raise();

        navBar->appear(scene());
        MOnDisplayChangeEvent* event = new MOnDisplayChangeEvent(true, m_navigationBarWindow->sceneRect());
        scene()->sendEvent(navBar, event);

        statusBar->appear(scene());
    }
    lower();
    hide();
}

void QtMaemo6ApplicationWindow::reposition()
{
    M::OrientationAngle currentAngle = M::Angle0;
    if(m_style && m_style->isOrientationChangeEnabled())
        currentAngle = m_style->orientation();
    
    //Orientation 90 and 180 should not be supported, so we skip them
    if(currentAngle == M::Angle90 || currentAngle == M::Angle180)
        return;
    
    setOrientationAngle(currentAngle);
    setOrientationAngleLocked(true);

    m_topDecorationSpacer->setFixedHeight(0);
    m_bottomDecorationSpacer->setFixedHeight(0);
    m_leftDecorationSpacer->setFixedWidth(0);
    m_rightDecorationSpacer->setFixedWidth(0);

    if(m_mainWindow->windowState().testFlag(Qt::WindowFullScreen)) {
        //set the mainWindow layout to zero spacing, so that the central widget becomes really fullscreen
        m_mainWindow->layout()->setSpacing(0);
        m_navigationBarWindow->hide();
        m_statusBarWindow->hide();
        hide();

        return;
    }

    QSize screenSize = MDeviceProfile::instance()->resolution();

    //setup statusbar
    const MStatusBarStyle* statusBarStyle =
            static_cast<const MStatusBarStyle *>(QtMaemo6StylePrivate::mStyle(QStyle::State_Active,
                    "MStatusBarStyle"));
    int statusBarHeight = 30;
    if(statusBarStyle) {
        statusBarHeight = statusBarStyle->minimumSize().height();
    }
    switch(currentAngle) {
        case M::Angle0:
            m_statusBarWindow->setGeometry(0, 0, screenSize.width(), statusBarHeight);
            m_topDecorationSpacer->setFixedHeight(statusBarHeight);
            break;
        case M::Angle90:
            m_statusBarWindow->setGeometry(screenSize.width() - statusBarHeight, 0, statusBarHeight, screenSize.height());
            m_rightDecorationSpacer->setFixedWidth(statusBarHeight);
            break;
        case M::Angle180:
            m_statusBarWindow->setGeometry(0, screenSize.height() - statusBarHeight, screenSize.width(), statusBarHeight);
            m_bottomDecorationSpacer->setFixedHeight(statusBarHeight);
            break;
        case M::Angle270:
            m_statusBarWindow->setGeometry(0, 0, statusBarHeight, screenSize.height());
            m_leftDecorationSpacer->setFixedWidth(statusBarHeight);
            break;
    }

    //setup navigationbar (== visible part of this window)
    const MNavigationBarStyle *navigationBarStyle =
        static_cast<const MNavigationBarStyle *>(QtMaemo6StylePrivate::mStyle(QStyle::State_Active,
                "MNavigationBarStyle"));

    int navigationBarHeight = 90;
    if(navigationBarStyle) {
        navigationBarHeight = navigationBarStyle->minimumSize().height();
        //if the status bar has align bottom, it's separated from the statusbar
        if(navigationBarStyle->verticalAlign() == Qt::AlignBottom) {
            switch(currentAngle) {
                case M::Angle0:
                    m_navigationBarWindow->setGeometry(0, screenSize.height() - navigationBarHeight, screenSize.width(), navigationBarHeight);
                    m_bottomDecorationSpacer->setFixedHeight(navigationBarHeight);
                    break;
                case M::Angle90:
                    m_navigationBarWindow->setGeometry(0, 0, navigationBarHeight, screenSize.height());
                    m_leftDecorationSpacer->setFixedWidth(navigationBarHeight);
                    break;
                case M::Angle180:
                    m_navigationBarWindow->setGeometry(0, 0, screenSize.width(), navigationBarHeight);
                    m_topDecorationSpacer->setFixedHeight(navigationBarHeight);
                    break;
                case M::Angle270:
                    m_navigationBarWindow->setGeometry(screenSize.width() - navigationBarHeight, 0, navigationBarHeight, screenSize.height());
                    m_rightDecorationSpacer->setFixedWidth(navigationBarHeight);
                    break;
            }
        } else {
            switch(currentAngle) {
                case M::Angle0:
                    m_navigationBarWindow->setGeometry(0, statusBarHeight, screenSize.width(), navigationBarHeight);
                    m_topDecorationSpacer->setFixedHeight(statusBarHeight + navigationBarHeight);
                    break;
                case M::Angle90:
                    m_navigationBarWindow->setGeometry(screenSize.width() - (navigationBarHeight + statusBarHeight), 0, navigationBarHeight, screenSize.height());
                    m_rightDecorationSpacer->setFixedWidth(statusBarHeight + navigationBarHeight);
                    break;
                case M::Angle180:
                    m_navigationBarWindow->setGeometry(0, screenSize.height() - (statusBarHeight + navigationBarHeight), screenSize.width(), navigationBarHeight);
                    m_bottomDecorationSpacer->setFixedHeight(statusBarHeight + navigationBarHeight);
                    break;
                case M::Angle270:
                    m_navigationBarWindow->setGeometry(statusBarHeight, 0, navigationBarHeight, screenSize.height());
                    m_leftDecorationSpacer->setFixedWidth(statusBarHeight + navigationBarHeight);
                    break;
            }
        }
    }

    setGeometry(QRect(QPoint(0,0), screenSize));
    setSceneRect(geometry());

    m_statusBarWindow->setSceneRect(m_statusBarWindow->geometry());
    m_navigationBarWindow->setSceneRect(m_navigationBarWindow->geometry());
}

void QtMaemo6ApplicationWindow::showMenu()
{
    //don't show menu if there are no contents.
    if(!m_mainWindow)
        return;
    if(!m_mainWindow->menuBar()->actions().count())
        return;
    else {
        if(!m_menuIsVisisble) {
            //switch to fullscreen view of MApplicationWindow, BEFORE the menu is about to show
            showFullScreen();
            //now show trigger menu manually, as we disconnected everything before
            openMenu();
            foreach(QGraphicsItem* item, items()) {
                MApplicationMenu* menu = dynamic_cast<MApplicationMenu*>(item);
                if(menu) {
                    buildMenuTree();
                    if(m_menuPage->actions().count()) {
                        m_menuIsVisisble = true;
                        connect(menu, SIGNAL(disappeared()), SLOT(hideMenu()));
                    }
                    break;
                }
            }
        } else { //if the menu is already visible, close it when menu button is clicked
            hideMenu();
        }
    }
}

void QtMaemo6ApplicationWindow::hideMenu()
{
    closeMenu();
    if(currentPage() == m_menuPage) {
        showNavBarOnly();
        m_menuIsVisisble = false;
    }
}

void QtMaemo6ApplicationWindow::hideSubMenu() {
    m_menuPage->appear(this);
    showNavBarOnly();
}

void QtMaemo6ApplicationWindow::buildMenuTree() {
    if(m_mainWindow && m_mainWindow->menuBar()) {

        foreach(QAction* action, m_menuPage->actions()) {
            if(qobject_cast<MAction*>(action)) {
                m_menuPage->removeAction(action);
                delete action;
            }
        }

        QList<QAction*> actionList = m_mainWindow->menuBar()->actions();
        foreach(QAction* action, actionList) {
            //only show enabled options
            if(action->isEnabled() && !action->isSeparator()) {
                MAction* maction = new MAction(action->text().remove(QChar('&')), navBar);
                maction->setLocation(MAction::ApplicationMenuLocation);
                maction->setMenu(action->menu());
                maction->setCheckable(action->isCheckable());
                if(maction->menu()) {
                    connect(maction, SIGNAL(triggered()), this, SLOT(showSubMenu()));
                } else {
                    connect(maction, SIGNAL(triggered()), action, SIGNAL(triggered()));
                    connect(maction, SIGNAL(triggered()), this, SLOT(hideMenu()));
                }
                m_menuPage->addAction(maction);
            }
        }
    } else
        mDebug("PlainQt Style") << "Trying to build menu tree without MainWindow or MenuBar.";
}

void QtMaemo6ApplicationWindow::showSubMenu() {
    if(m_menuIsVisisble) {
        hideMenu();
    }
    MAction* action = qobject_cast<MAction*>(sender());
    if(action && action->menu()) {
        //clear the layout
        while(m_subMenuPageLayout->count())
            delete m_subMenuPageLayout->takeAt(0);

        foreach(QAction* action, action->menu()->actions()) {
            if(action->isEnabled() && !action->isSeparator()) {
                MButton* button = new MButton(action->text().remove(QChar('&')));
                button->addAction(action);
                connect(button, SIGNAL(clicked()), this, SLOT(handleSubMenuAction()));
                m_subMenuPageLayoutPolicy->addItem(button, Qt::AlignCenter);
            }
        }

        m_subMenuPage->appear(this);
        showFullScreen();
    }
}

void QtMaemo6ApplicationWindow::handleSubMenuAction() {
    //ensure that the subMenuPage dissapears before the action is triggered
    // otherwise the application page will minimize and an possibly opened
    // page will be not shown.
    m_subMenuPage->disappear();
    if(MButton* button = qobject_cast<MButton*>(sender())) {
        if(button->actions().count()) {
            if(QAction* action = button->actions().first()) {
                action->trigger();
            }
        }
    }
}

void QtMaemo6ApplicationWindow::disappearCurrentPage() {
    if(currentPage()) {
        currentPage()->disappear();
    }
}

void QtMaemo6ApplicationWindow::orientationChange(M::OrientationAngle) {
}

void QtMaemo6ApplicationWindow::mousePressEvent(QMouseEvent *event) {
    MApplicationWindow::mousePressEvent(event);
}

/*
void QtMaemo6ApplicationWindow::minimize()
{
    qCritical()<<"minimize";

    if(!parentWidget())
        return;

    XEvent e;
    memset(&e, 0, sizeof(e));

    e.xclient.type = ClientMessage;
    e.xclient.message_type = XInternAtom(QX11Info::display(), "WM_CHANGE_STATE",
                                         False);
    e.xclient.display = QX11Info::display();
    e.xclient.window = parentWidget()->window()->winId();
    e.xclient.format = 32;
    e.xclient.data.l[0] = IconicState;
    e.xclient.data.l[1] = 0;
    e.xclient.data.l[2] = 0;
    e.xclient.data.l[3] = 0;
    e.xclient.data.l[4] = 0;
    XSendEvent(QX11Info::display(), QX11Info::appRootWindow(),
               False, (SubstructureNotifyMask | SubstructureRedirectMask), &e);

    XSync(QX11Info::display(), FALSE);
}
*/

bool QtMaemo6ApplicationWindow::eventFilter(QObject* object, QEvent* event) {
    if(qobject_cast<QMainWindow*>(object) == m_mainWindow) {
        if(event->type() == QEvent::WindowActivate) {
            setWindowVisibility(effectiveWinId(), true);
        }
        if(event->type() == QEvent::WindowStateChange) {
            setWindowVisibility(effectiveWinId(), m_mainWindow->isVisible());
            //window state might have changed Qt::WindowFullScreen, so we have to update visibility of status- and navigationbarwindow
            QWindowStateChangeEvent* wsce = static_cast<QWindowStateChangeEvent*>(event);
            bool oldFullscreenState = wsce->oldState().testFlag(Qt::WindowFullScreen);
            bool newFullScreenState = m_mainWindow->windowState().testFlag(Qt::WindowFullScreen);

            if(oldFullscreenState != newFullScreenState) {
                reposition();
                if(!m_mainWindow->windowState().testFlag(Qt::WindowFullScreen))
                    showNavBarOnly();
            }
        }
    }
    return MApplicationWindow::eventFilter(object, event);
}



