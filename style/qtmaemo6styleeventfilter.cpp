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

#include "qtmaemo6styleeventfilter.h"
#include "qtmaemo6style_p.h"
#include "qtmaemo6dialogdimmer.h"
#include "qtmaemo6comboboxpopup.h"
#include "qtmaemo6applicationwindow.h"

#include <QApplication>
#include <QWidget>
#include <QEvent>
#include <QKeyEvent>
#include <QLayout>
#include <QMainWindow>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QToolButton>
#include <QLineEdit>
#include <QTreeView>
#include <QComboBox>
#include <QMenu>
#include <QDir>
#include <QDate>
#include <QTime>
#include <QDesktopWidget>
#include <QStyleOptionComboBox>

#include "qtscroller.h"
#include "qtscrollevent.h"

#include <MComponentData>
#include <mfeedback.h>
#include <mdeviceprofile.h>
#include <MTheme>
#include <mbuttonstyle.h>
#include <MWindow>
#include <MDebug>

#include "qtmaemo6clicklabel.h"

#ifdef HAVE_MEEGOGRAPHICSSYSTEM
#include <QtMeeGoGraphicsSystemHelper>
#endif

//krazy:excludeall=qclasses

static QMap<QObject*, QtMaemo6DialogDimmer*> g_dimmerMap;
QStringList QtMaemo6StyleEventFilter::nonScrollableWidgets = QStringList();

QtMaemo6StyleEventFilter::QtMaemo6StyleEventFilter(QtMaemo6Style *parent)
    : QObject(parent)
{
    m_style = parent;
    nonScrollableWidgets << "QLineEdit";
}


bool QtMaemo6StyleEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    QWidget *widget = qobject_cast<QWidget *>(obj);

    switch (event->type()) {
    case QEvent::ToolTip: {
        //skip tooltips as they make no sense on a touchscreen device
        return true;
    }
    break;
    case QEvent::Show: {
        if (NULL != widget) {
            if (widget->isWindow() && !widget->graphicsProxyWidget()) {
                if (QDialog *dialog = qobject_cast<QDialog *>(widget)) {
                    // set background
                    if(QtMaemo6Style* style = qobject_cast<QtMaemo6Style*>(dialog->style()))
                    {
                        style->setPaletteBackground(dialog);
                    }

                    QtMaemo6DialogDimmer* dimmer = g_dimmerMap.value( dialog, 0 );

                    if ( ! dimmer )
                    {
                        dimmer = new QtMaemo6DialogDimmer( dialog );
                        g_dimmerMap.insert( dialog, dimmer );
                        connect( dimmer, SIGNAL( dimmerClicked() ), dialog, SLOT( reject() ) );
                        // make sure dimmer is deleted when dialog is deleted.
                        connect( dialog, SIGNAL( destroyed() ), this, SLOT( dialogDestroyed() ) );
                    }

                    dimmer->show();

                    if (dialog->isModal())
                    {
                        // Remove modality so the dimmer can get click events to close the dialog.
                        // This can safely be done because the only window/widget that can receive
                        // events now is our DialogDimmer. It will use click events and call
                        // reject() on the corresponding dialog and throw away all other events,
                        // so the overall behaviour is exactly like a normal modal dialog.
                        dialog->setModal(false);
                    }
                }
            }
            // Unfortunately this can't be safely done in polish:
            if (QDialogButtonBox *box = qobject_cast<QDialogButtonBox *>(widget)) {
                QPushButton *okButton = box->button(QDialogButtonBox::Ok);
                //~ uispec-document DirectUI_Common_Strings_UI_Specification_0.7.doc
                //: Command for confirmation
                //% "Done"
                if (okButton) okButton->setText(qtTrId("qtn_comm_command_done"));
                QPushButton *cancelButton = box->button(QDialogButtonBox::Cancel);
                if (cancelButton) cancelButton->hide();
            }
            if (QPushButton *button = qobject_cast<QPushButton *>(widget)) {
                QString okString = tr("OK");
                //~ uispec-document DirectUI_Common_Strings_UI_Specification_0.7.doc
                //: Command for confirmation
                //% "Done"
                QString doneString = qtTrId("qtn_comm_command_done");
                if (button->text() == okString) button->setText(doneString);
                QString cancelString = tr("Cancel");
                if (button->text() == cancelString) {
                    button->hide();
                }
            }
            if(QLineEdit* le = qobject_cast<QLineEdit*>(widget)) {
                //poor way to hide the lineedit of a combobox if it is editable
                if(qobject_cast<QComboBox*>(le->parentWidget())) {
                    le->setVisible(false);
                    return true;
                }
            }
            if (QTreeView *treeWidget = qobject_cast<QTreeView *>(widget)) {
                //unfortunately this must be done in showEvent, because it's
                // a property of QTreeWidget, that can't be influenzed by the
                // style

                //FIXME: really bad to do all that in showEvent
                QToolButton button;
                QStyleOptionToolButton option;
                option.initFrom(&button);
                option.text = '+';
                QSize size = m_style->sizeFromContents(QStyle::CT_ToolButton, &option, QSize(), &button);

                //only set the indentation if the required buttonsize is bigger
                // than original indentation
                if (size.width() > treeWidget->indentation()) {
                    treeWidget->setIndentation(size.width());
                }
            }

            /*
            if(QtMaemo6Style* style = qobject_cast<QtMaemo6Style*>(widget->style())) {
                style->delayedPolish(widget);
            }
            */
        }

    }
    break;
    case QEvent::Hide: {
        // hide the dimmer widget when the dialog gets hidden
        if (NULL != widget) {
            if (widget->isWindow() && !widget->graphicsProxyWidget()) {
                QtMaemo6DialogDimmer *pDimmer = g_dimmerMap.value( widget, 0 );

                if( pDimmer )
                {
                    pDimmer->hide();
                }
            }
        }
    }
    break;
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress: {
        // now send press feedback
        if(QtMaemo6Style* style = qobject_cast<QtMaemo6Style*>(widget->style())) {
            const MFeedback *  feedback = style->feedback(MFeedback::Press, widget);
            if ( feedback ) feedback->play();

            if (style->d_func()->m_pressedComboBox)
                style->d_func()->m_pressedComboBox = 0;

            //draw button in sunken mode
            if(QComboBox *comboBox = qobject_cast<QComboBox*>(widget)) {
                style->d_func()->m_pressedComboBox = comboBox;
                comboBox->update();
                //suppress mouse press event for combobox, i.e. don't show combobox popup field
                return true;
            }

        }

    }
    break;
    case QEvent::MouseButtonRelease: {

        //don't keep the current tab in sunken state  when mouse is released
        if (qobject_cast<QTabBar *>(widget))
            widget->update();

        //check for faked gesture events
        QMouseEvent *mre = static_cast<QMouseEvent *>(event);
        if (mre && mre->globalPos() == QPoint(-QWIDGETSIZE_MAX, -QWIDGETSIZE_MAX)) {
            if(QtMaemo6Style* style = qobject_cast<QtMaemo6Style*>(widget->style()))
                style->d_func()->m_pressedComboBox = 0;
            break;
        }

        // now send release feedback
        if(QtMaemo6Style* style = qobject_cast<QtMaemo6Style*>(widget->style())) {
            const MFeedback *  feedback = style->feedback(MFeedback::Release, widget);
            if ( feedback ) feedback->play();
            if(QComboBox* comboBox = qobject_cast<QComboBox*>(widget)) {
                if (style->d_func()->m_pressedComboBox && style->d_func()->m_pressedComboBox == comboBox) {
                   QtMaemo6ComboBoxPopup *comboBoxPopup = new QtMaemo6ComboBoxPopup(comboBox, NULL);
                   comboBoxPopup->setFixedSize(MDeviceProfile::instance()->resolution());
                   comboBoxPopup->show();
                   style->d_func()->m_pressedComboBox = 0;
                   return true;
               }
                style->d_func()->m_pressedComboBox = 0;
            }
        }

    }
    break;
    case QEvent::MouseMove: {
        if(QtMaemo6Style* style = qobject_cast<QtMaemo6Style*>(widget->style())) {
            if (QComboBox *comboBox = qobject_cast<QComboBox *>(widget)) {
                comboBox->rect().contains(((QMouseEvent*)event)->pos()) ?
                        style->d_func()->m_pressedComboBox = comboBox : style->d_func()->m_pressedComboBox = 0;
                comboBox->update();
            }
        }
    }
    break;
    case QEvent::Wheel: {
        if(qobject_cast<QComboBox*>(widget)) {
            return true;
        }
    }
    break;
    case QEvent::FocusIn:
    case QEvent::FocusOut: {
        if(QtMaemo6Style* style = qobject_cast<QtMaemo6Style*>(widget->style())) {
           if(qobject_cast<QLineEdit*>(widget)) {
               style->d_func()->setPaletteTextColor(widget);
           }
       }
    }
    break;
    case QEvent::KeyPress: {
        QKeyEvent* k = static_cast<QKeyEvent*>(event);
        if(Qt::Key_T == k->key() && k->modifiers () & (Qt::ControlModifier | Qt::AltModifier)
            && (k->modifiers () & Qt::ShiftModifier)) {
            mDebug("PlainQt Style") << "Taking screenshot";
            QPixmap screenshot;
            screenshot = QPixmap::grabWindow(QApplication::desktop()->effectiveWinId());

            QString path = QDir::homePath() + "/MyDocs/.images";
            if (!QDir(path).exists())
                path = QDir::homePath();

            QString imageFile = QString("%1/%2-%3.png").arg(path)
                .arg(QDate::currentDate().toString("yyyyMMdd"))
                .arg(QTime::currentTime().toString("hhmmss"));
            if (!screenshot.save(imageFile))
                mDebug("PlainQt Style") << "Could not save screenshot to" << imageFile;
            else
                mDebug("PlainQt Style") << "Screenshot saved to" << imageFile;
            return true;
        }
    }
    break;
    case QtScrollPrepareEvent::ScrollPrepare: {

            //dont's scroll on any widget
            if (obj->isWidgetType()) {
                QWidget *scroller = qobject_cast<QWidget *>(obj);
                QtScrollPrepareEvent *scrollEvent = static_cast<QtScrollPrepareEvent *>(event);
                QPoint clickedAt = scrollEvent->startPos().toPoint();
                if (scroller->childAt(clickedAt) && nonScrollableWidgets.contains(scroller->childAt(clickedAt)->metaObject()->className()))
                    return true;
            }
        }
    break;
#ifdef HAVE_MEEGOGRAPHICSSYSTEM
    case QEvent::WindowStateChange: {
        if(QMainWindow* mw = qobject_cast<QMainWindow*>(widget)) {
            if (QMeeGoGraphicsSystemHelper::runningGraphicsSystemName() != QLatin1String("native")) {
                if(!mw->windowState().testFlag(Qt::WindowMinimized)) {
                    if (!QMeeGoGraphicsSystemHelper::isRunningMeeGo()) {
                        mDebug("MGraphicsSystemHelper") << "Switching to hardware rendering with meego graphicssystem";
                        QMeeGoGraphicsSystemHelper::switchToMeeGo();
                    }
                } else {
                    if (QMeeGoGraphicsSystemHelper::isRunningMeeGo()) {
                        mDebug("MGraphicsSystemHelper") << "switching to raster";
                        QMeeGoGraphicsSystemHelper::switchToRaster();
                    }
                }
            }
        }
    }
    break;
#endif
    default:
        break;
    }
    // standard event processing
    return QObject::eventFilter(obj, event);
}

void QtMaemo6StyleEventFilter::dialogDestroyed()
{
    // here we delete the dialog from the map, when it gets deleted

    QObject *pDialog = sender();
    QObject *pDimmer = g_dimmerMap.value( pDialog, 0 );

    if ( pDimmer )
    {
        delete pDimmer;
        g_dimmerMap.remove( pDialog );
    }
}
