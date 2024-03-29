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


#include "qtmaemo6style.h"
#include "qtmaemo6style_p.h"
#include "qtmaemo6applicationwindow.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QListWidget>
#include <QTextEdit>
#include <QPainter>
#include <QProgressBar>
#include <QStyleOptionButton>

#include <QDialogButtonBox>
#include <QFile>
#include <QInputContext>
#include <QInputContextFactory>
#include <QPoint>
#include <QToolBar>
#include <QMainWindow>
#include <QDialog>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QMoveEvent>
#include <QSequentialAnimationGroup>
#include <QPixmapCache>
#include <QPropertyAnimation>
#include <QPauseAnimation>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QAbstractItemView>
#include <QStatusBar>
#include <QMenuBar>
#include <QComboBox>
#include <QDesktopWidget>
#include <QLineEdit>
#include <QTime>
#include <QHeaderView>
#include <QToolBox>

#include <MComponentData>
#include <MDebug>
#include <MFeedback>
#include <MTheme>
#include <MScalableImage>
#include <MLabel>
#include <MImageWidget>
#include <MApplicationWindow>
#include <MKeyboardStateTracker>
#include <MInputMethodState>
#include <mbuttonstyle.h>
#include <mapplicationpagestyle.h>
#include <mpositionindicatorstyle.h>
#include <mprogressindicatorstyle.h>
#include <mpannablewidgetstyle.h>
#include <mwidgetstyle.h>
#include <mdialogstyle.h>
#include <mbuttoniconstyle.h>
#include <mlistitemstyle.h>
#include <mcheckboxstyle.h>
#include <mcontainerstyle.h>
#include <mcomboboxstyle.h>
#include <mlabelstyle.h>
#include <mimagewidgetstyle.h>
#include <mnavigationbarstyle.h>
#include <mseparatorstyle.h>
#include <msliderstyle.h>
#include <mtexteditstyle.h>
#include <mcontentitemstyle.h>
#include <mapplicationmenustyle.h>
#include <mfeedback.h>
#include <mdeviceprofile.h>
#include <morientationtracker.h>
#include <mcombobox.h>

#include "qtscroller.h"

#include "qtmaemo6titlebar.h"
#include "qtmaemo6sliderpopup.h"
//krazy:excludeall=qclasses

//#include "mcontainerheader_p.h"

#define WIDGET_OPACITY "widgetOpacity"
#define SCROLLAREATHUMBVIEW "scrollAreaThumbView"

//#define MOVE_ACTIONS_FROM_TOOLBAR_TO_TITLEBAR
//#define M_LOG_POLISH


QPixmap setPixmapOpacity(const QPixmap &pixmap, double opacity)
{
    //opacity == 1 makes no transparency, so let the pixmap unchanged
    if (opacity >= 1.0)
        return pixmap;

    QPixmap transparentPixmap(pixmap.size());
    transparentPixmap.fill(Qt::transparent);

    //if the opacity is 0, just return a fully transparent pixmap
    if (opacity >= 0.0) {
        QPainter painter(&transparentPixmap);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.drawPixmap(0, 0, pixmap);

        painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        painter.fillRect(transparentPixmap.rect(), QColor(0, 0, 0, 255 * opacity));
        painter.end();
    }
    return transparentPixmap;
}

QtMaemo6StylePrivate::QtMaemo6StylePrivate()
    : m_actionsInTitleBarCount(5),
      m_componentData(0),
#ifdef HAVE_CONTEXTSUBSCRIBER
      m_orientation("Screen.TopEdge"),
#endif //HAVE_CONTEXTSUBSCRIBER
      m_isMInitialized(false),
      m_isMApplication(false),
      m_isOrientationChangeEnabled(false),
      m_pressedComboBox(0),
      m_scrollBarEventFilter(0),
      m_windowEventFilter(0),
      m_menuBar(0),
      q_ptr(0)
{
    m_excludeClasses.append("QShapedPixmapWidget");
    m_excludeClasses.append("QTipLabel");
}

QtMaemo6StylePrivate::~QtMaemo6StylePrivate()
{
    if (!m_isMApplication) {
        delete m_componentData;
    }
}

void QtMaemo6StylePrivate::initM()
{
    mDebug("PlainQt Style") << "Qt Maemo 6 Style init";
    
    Q_Q(QtMaemo6Style);

    m_isMInitialized = true;

    m_windowEventFilter = new QtMaemo6StyleEventFilter(q);
    m_scrollBarEventFilter = new QtMaemo6ScrollBarEventFilter(q);
    m_scrollBarEventFilter->setScrollBarsAlwaysVisible(false);

    if (MComponentData::instance() != 0) {
        m_componentData = MComponentData::instance();
        m_isMApplication = true;
    } else {
        QStringList args = qApp->arguments();

        int argc = args.count();
        char *argv[ args.count() ];

        for(int i = 0; i < args.count(); i++) {
             argv[i] = qstrdup(args[ i ].toLocal8Bit().constData());
        }
        m_componentData = new MComponentData(argc, argv);
    }

    const MLabelStyle *style =
        static_cast<const MLabelStyle *>(mStyle(QStyle::State_Active,
                                           "MLabelStyle"));
    qApp->setFont(style->font());
    qApp->setGlobalStrut(QSize(0, 0));

    bool inputConnect = QObject::connect(MInputMethodState::instance(), SIGNAL(inputMethodAreaChanged(QRect)), q, SLOT(ensureFocusedWidgetVisible(QRect)));
    if(!inputConnect)
        mDebug("PlainQt Style") << "Virtual keyboard notification connection failed";

    QObject::connect(MTheme::instance(), SIGNAL(pixmapRequestsFinished()), q, SLOT(updateDirtyWidgets()));

    //Force VKB to landscape orientation
    MInputMethodState::instance()->setActiveWindowOrientationAngle(M::Angle0);
}

const MStyle *QtMaemo6StylePrivate::mStyle(QStyle::State state,
        const QString &styleClass,
        const QString &styleObject,
        const QString &type,
        const bool hasFocus,
        const MWidgetController *parent
                                              )
{
    // Set mode
    QString mode = modeFromState(state);

    // a line edit is selected in respect of its focus
    if ((styleClass == "MTextEditStyle") && hasFocus)
        mode = "selected";

    const MStyle* style = MTheme::style(styleClass.toLocal8Bit().constData(),
                                        styleObject.toLocal8Bit().constData(),
                                        mode, type, M::Landscape, parent);
    if(!style) {
        qCritical() << "Can not load MStyle" << styleClass << styleObject << type;
    }
    return style;
}

QString QtMaemo6StylePrivate::modeFromState(QStyle::State state)
{
    // Used for debugging the QStyle::State vs. mode behavior:
    // printStateFlags(state);

    // At moment, libmeegotouch uses the following modes:
    // "" (empty string)        -- used as default
    // "pressed"                -- used whenever a widget is pressed
    // "selected"               -- used by checkable elements (for example checkboxes)
    // vertical", "horizontal"  -- used by MSeparatorStyle
    QString mode = "";

    if (state & QStyle::State_Enabled) {

        if(state & QStyle::State_Sunken)
            mode = "pressed";
        else if(state & QStyle::State_On)
            mode = "selected";
        else if(state & QStyle::State_Selected)
            mode = "selected";
        else if(state & QStyle::State_Raised)
            mode = "";

    }

    return mode;
}

void QtMaemo6StylePrivate::printStateFlags(QStyle::State state)
{

   if (state & QStyle::State_None)
        mDebug("PlainQt Style") << "QStyle::State_None";

    if (state & QStyle::State_Active)
        mDebug("PlainQt Style") << "QStyle::State_Active";

    if (state & QStyle::State_AutoRaise)
        mDebug("PlainQt Style") << "QStyle::State_AutoRaise";

    if (state & QStyle::State_Children)
        mDebug("PlainQt Style") << "QStyle::State_Children";

    if (state & QStyle::State_DownArrow)
        mDebug("PlainQt Style") << "QStyle::State_DownArrow";

    if (state & QStyle::State_Editing)
        mDebug("PlainQt Style") << "QStyle::State_Editing";

    if (state & QStyle::State_Enabled)
        mDebug("PlainQt Style") << "QStyle::State_Enabled";

//    if (state & QStyle::State_HasEditFocus)
//      mDebug("PlainQt Style") << "QStyle::State_HasEditFocus";

    if (state & QStyle::State_Horizontal)
        mDebug("PlainQt Style") << "QStyle::State_Horizontal";

    if (state & QStyle::State_KeyboardFocusChange)
        mDebug("PlainQt Style") << "QStyle::State_KeyboardFocusChange";

    if (state & QStyle::State_MouseOver)
        mDebug("PlainQt Style") << "QStyle::State_MouseOver";

    if (state & QStyle::State_NoChange)
        mDebug("PlainQt Style") << "QStyle::State_NoChange";

    if (state & QStyle::State_Off)
        mDebug("PlainQt Style") << "QStyle::State_Off";

    if (state & QStyle::State_On)
        mDebug("PlainQt Style") << "QStyle::State_On";

    if (state & QStyle::State_Raised)
        mDebug("PlainQt Style") << "QStyle::State_Raised";

    if (state & QStyle::State_ReadOnly)
        mDebug("PlainQt Style") << "QStyle::State_ReadOnly";

    if (state & QStyle::State_Selected)
        mDebug("PlainQt Style") << "QStyle::State_Selected";

    if (state & QStyle::State_Item)
        mDebug("PlainQt Style") << "QStyle::State_Item";

    if (state & QStyle::State_Open)
        mDebug("PlainQt Style") << "QStyle::State_Open";

    if (state & QStyle::State_Sibling)
        mDebug("PlainQt Style") << "QStyle::State_Sibling";

    if (state & QStyle::State_Sunken)
        mDebug("PlainQt Style") << "QStyle::State_Sunken";

    if (state & QStyle::State_UpArrow)
        mDebug("PlainQt Style") << "QStyle::State_UpArrow";

    if (state & QStyle::State_Mini)
        mDebug("PlainQt Style") << "QStyle::State_Mini";

    if (state & QStyle::State_Small)
        mDebug("PlainQt Style") << "QStyle::State_Small";

}

bool QtMaemo6Style::setPaletteBackground(QWidget *widget,
                                         QString styleObject,
                                         QString styleClass) const
{
    bool ret = false;

    if (NULL != widget) {

        //ensure the layout was activated, so the widget already has correct size
        if(widget->parentWidget())
            if(widget->parentWidget()->layout())
                widget->parentWidget()->layout()->activate();

        if(widget->size().isValid()) {
            widget->setAutoFillBackground(true);

            QStyleOption widgetOption;
            widgetOption.initFrom(widget);

            QPixmap backgroundPixmap(widget->size());
            backgroundPixmap.fill(Qt::transparent);
            QPainter painter(&backgroundPixmap);

            //by default use the MApplicationPageStyle background
            QString _styleObject("MApplicationPageStyle");
            if(!styleObject.isEmpty())
                _styleObject = styleObject;

            const MWidgetStyle *style =
                static_cast<const MWidgetStyle*>(
                    QtMaemo6StylePrivate::mStyle(widgetOption.state, _styleObject, styleClass));
            if(style) {
                ret = drawBackground(&painter, &widgetOption, backgroundPixmap.rect(), style, widget);

                if(ret) {
                    QPalette palette = widget->palette();
                    palette.setBrush(widget->backgroundRole(), QBrush(backgroundPixmap));
                    widget->setPalette(palette);
                }
            }
        }
    }
    return ret;
}

bool QtMaemo6Style::setPaletteBackground(QWidget* widget,
                                         const MScalableImage* image) const
{
    bool ret = false;
    if(widget && image) {
        Q_D(const QtMaemo6Style);
        //this is needed to force the widget to draw its palette background
        // autmatically before each paint event
        widget->setAutoFillBackground(true);

        QStyleOption widgetOption;
        widgetOption.initFrom(widget);

        QPixmap backgroundPixmap(widget->size());
        backgroundPixmap.fill(Qt::transparent);
        QPainter painter(&backgroundPixmap);

        ret = d->drawScalableImage(&painter, &widgetOption, backgroundPixmap.rect(),
                                   image, NULL, widget);
        if(ret) {
            QPalette palette = widget->palette();
            palette.setBrush(widget->backgroundRole(), QBrush(backgroundPixmap));
            widget->setPalette(palette);
        }
    }
    return ret;
}

bool QtMaemo6Style::drawBackground(QPainter *p,
                                   const QStyleOption *option,
                                   const QRect &rect,
                                   const MWidgetStyle *style,
                                   const QWidget* w /* = 0*/) const
{
    Q_D(const QtMaemo6Style);
    bool ret = false;
    if (style) {
        if(style->backgroundImage()) {
            ret = d->drawScalableImage(p, option, rect, style->backgroundImage(), style, w);
        } else if(style->backgroundColor().isValid()) {
            p->fillRect(rect, QBrush(style->backgroundColor()));
        }
    }
    return ret;
}

bool QtMaemo6StylePrivate::drawScalableImage(QPainter *p,
        const QStyleOption *option,
        const QRect &rect,
        const MScalableImage *scalableImage,
        const MWidgetStyle *style,
        const QWidget* widget,
        const QString &purpose,
        bool enableCache) const
{
    qreal effectiveOpacity = p->opacity();
    if(style)
        p->setOpacity(style->backgroundOpacity() * effectiveOpacity);
    if (!scalableImage) {
        qWarning() << "Image not valid *********************";
    }
    if (scalableImage && rect.isValid()) {
        if (MTheme::hasPendingRequests()) {
            //cache widgets that need an update, after the images are loaded
            QWidget* usedWidget = const_cast<QWidget*>(widget);

            //if no widget is provieded, use the painter's device, if painted onto a widget
            if(!usedWidget) {
                usedWidget = dynamic_cast<QWidget*>(p->device());
            }
            if(usedWidget) {
                if(!m_dirtyWidgets.contains(usedWidget)) {
                    MScalableImage* imageForBackground = 0;
                    if(widget)
                        imageForBackground = const_cast<MScalableImage*>(scalableImage);
                    m_dirtyWidgets.insert(usedWidget, imageForBackground);
                }
            }
            return false;
        }

        // Per Widget background image cache implementation
        QPixmap backgroundPixmap(rect.size());

        QPaintDevice *device = p->device();
        QWidget *cachedWidget = dynamic_cast<QWidget *>(device);

        QString mode;
        if (cachedWidget) {
            mode = QtMaemo6StylePrivate::modeFromState(option->state);
        }

        quintptr pWidget = reinterpret_cast<quintptr>(device);
        quintptr pImage = reinterpret_cast<quintptr>(scalableImage);

        QString cacheKey = QString("%1_%2_%3_%4_%5x%6").arg(pWidget)
                           .arg(pImage)
                           .arg(mode)
                           .arg(purpose)
                           .arg(rect.width())
                           .arg(rect.height());

        if ((!enableCache || !QPixmapCache::find(cacheKey, backgroundPixmap)) && rect.isValid()) {
            backgroundPixmap.fill(Qt::transparent);
            QPainter pixmapPainter(&backgroundPixmap);
            scalableImage->draw(0, 0, rect.width(), rect.height(), &pixmapPainter);
            QPixmapCache::insert(cacheKey, backgroundPixmap);
        }

        if (widget && widget->dynamicPropertyNames().contains(WIDGET_OPACITY)) {
            backgroundPixmap = setPixmapOpacity(backgroundPixmap, widget->property(WIDGET_OPACITY).toDouble());
        }

        p->drawPixmap(rect, backgroundPixmap);
    } else if (style && style->backgroundColor().isValid()) {
        p->fillRect(rect, QBrush(style->backgroundColor()));
    }
    p->setOpacity(effectiveOpacity);

    return true;
}

void QtMaemo6StylePrivate::drawSliderBaseBackground(QPainter *p,
        const QStyleOption *option,
        const QRect &rect,
        const MSliderStyle *style,
        int maxSliderLength)
{
    Q_UNUSED(maxSliderLength);

    p->save();
    if (style) {
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            // draw Background
            qreal effectiveOpacity = p->opacity();
            p->setOpacity(style->backgroundOpacity() * effectiveOpacity);

            bool isHorizontal = slider->orientation == Qt::Horizontal;

            int span = ( isHorizontal )
                        ? slider->rect.width() - maxSliderLength
                        : slider->rect.height() - maxSliderLength;
            int sliderPosition = QStyle::sliderPositionFromValue(slider->minimum,
                                                                 slider->maximum,
                                                                 slider->sliderValue,
                                                                 span,
                                                                 slider->upsideDown);
            //adjust the slider position to the absolute position within the widget
            // instead of position on the groove
            if(isHorizontal)
                sliderPosition = ((rect.width() - span) / 2) + sliderPosition;
            else
                sliderPosition = ((rect.height() - span) / 2) + sliderPosition;

            const MScalableImage *baseImage = (isHorizontal)
                                                ? style->backgroundBaseImage()
                                                : style->backgroundVerticalBaseImage();

            const MScalableImage *elapsedImage = (isHorizontal)
                                                ? style->backgroundElapsedImage()
                                                : style->backgroundVerticalElapsedImage();

            if(qApp->isRightToLeft()) {
                const MScalableImage* tempImage = baseImage;
                baseImage = elapsedImage;
                elapsedImage = tempImage;
            }

            if (baseImage) {
                // Per Widget background image cache implementation
                QPixmap backgroundPixmap(rect.size());

                QPaintDevice *device = p->device();
                QWidget *cachedWidget = dynamic_cast<QWidget *>(device);

                QString mode;
                if (cachedWidget) {
                    mode = QtMaemo6StylePrivate::modeFromState(option->state);
                }

                quintptr pWidget = reinterpret_cast<quintptr>(device);

                QString cacheKey = QString("%1_%2_%3x%4_%5").arg(pWidget)
                                   .arg(mode)
                                   .arg(rect.width())
                                   .arg(rect.height())
                                   .arg(slider->sliderValue);

                if (!QPixmapCache::find(cacheKey, backgroundPixmap)) {
                    backgroundPixmap.fill(Qt::transparent);
                    QPainter pixmapPainter;
                    pixmapPainter.begin(&backgroundPixmap);
                    if(isHorizontal) {
                        if(sliderPosition > 0)
                            elapsedImage->draw(0, 0, sliderPosition, rect.height(), &pixmapPainter);
                        baseImage->draw(sliderPosition, 0, rect.width() - sliderPosition, rect.height(), &pixmapPainter);
                    } else {
                        if(sliderPosition > 0)
                            elapsedImage->draw(0, 0, rect.width(), sliderPosition, &pixmapPainter);
                        baseImage->draw(0, sliderPosition, rect.width(), rect.height() - sliderPosition, &pixmapPainter);
                    }
                    pixmapPainter.end();
                    QPixmapCache::insert(cacheKey, backgroundPixmap);
                }
                p->drawPixmap(rect, backgroundPixmap);
            } else if (style->backgroundColor().isValid()) {
                p->fillRect(rect, QBrush(style->backgroundColor()));
            }

            p->setOpacity(effectiveOpacity);
        }
    }
    p->restore();
}

void QtMaemo6StylePrivate::drawBasicButton(QPainter *painter,
        const QString &text,
        const QIcon &icon,
        const QRect &rect,
        const QStyleOption *option,
        const MButtonStyle *style) const
{
    drawBasicButton(painter, text, icon, rect, option, style, style->font(), style->iconSize());
}

void QtMaemo6StylePrivate::drawBasicButton(QPainter *p,
        const QString &text,
        const QIcon &icon,
        const QRect &rect,
        const QStyleOption *option,
        const QString &styleClass,
        const QString &styleObject /* = QString()*/) const
{
    const MButtonStyle *style =
        static_cast<const MButtonStyle *>(QtMaemo6StylePrivate::mStyle(option->state,
                                            styleClass.toLocal8Bit().constData(),
                                            styleObject));
    drawBasicButton(p, text, icon, rect, option, style, style->font(), style->iconSize());

}

void QtMaemo6StylePrivate::drawToggleButton(QPainter *p,
        const QString &text,
        const QIcon &icon,
        const QRect &rect,
        const QStyleOption *option,
        const QString &styleClass,
        const QString &styleObject /* = QString()*/) const
{
    const MButtonStyle *style =
        static_cast<const MButtonStyle *>(QtMaemo6StylePrivate::mStyle(option->state,
                                            styleClass.toLocal8Bit().constData(),
                                            styleObject,
                                            "toggle"));
    drawBasicButton(p, text, icon, rect, option, style, style->font(), style->iconSize());
}

void QtMaemo6StylePrivate::drawCheckBox(QPainter *p,
                                        const QString &text,
                                        const QIcon &icon,
                                        const QRect &rect,
                                        const QStyleOption *option) const
{
    const MCheckboxStyle *style =
        static_cast<const MCheckboxStyle *>(QtMaemo6StylePrivate::mStyle(option->state,
                "MCheckboxStyle"));
    if(style) {
        drawBasicButton(p, text, icon, rect, option, style, style->font(), style->iconSize());
        if(option->state & QStyle::State_On
          && option->state & QStyle::State_Enabled
          && style->checkmarkImage()) {
            QSizeF pos = (rect.size() / 2) - (style->checkmarkImage()->size() / 2);
            p->drawPixmap(pos.width() + rect.x(), pos.height() + rect.y(), *style->checkmarkImage());
        }
    } else {
        qCritical() << "Could not load MCheckboxStyle for QCheckBox";
    }
}

void QtMaemo6StylePrivate::drawBasicButton(QPainter *p,
        const QString &text,
        const QIcon &icon,
        const QRect &rect,
        const QStyleOption *option,
        const MButtonStyle *style,
        const QFont &font,
        const QSize &iconSize) const
{
    Q_Q(const QtMaemo6Style);
    if (style) {

        //margins aren't part of the button
        QRect buttonRect = rect.adjusted(style->marginLeft(), style->marginTop(), -style->marginRight(), -style->marginBottom());
        q->drawBackground(p, option, buttonRect, style);

        buttonRect.adjust(style->paddingLeft(), style->paddingTop(), -style->paddingRight(), -style->paddingBottom());
        QSize usedIconSize = iconSize.isValid() ? iconSize : style->iconSize();

        if (!icon.isNull()) {

            //scale icon if necessary (ToDo: investigate if m scales or cuts)
            if (usedIconSize.width() > buttonRect.width()
                    || usedIconSize.height() > buttonRect.height()) {
                usedIconSize.scale(buttonRect.size(), Qt::KeepAspectRatio);
            }

            if (text.isEmpty()) {
                drawButtonIcon(style, p, buttonRect, icon, usedIconSize, /*centerAlways=*/true);
            } else {

                    QRect textAndIconRect = getTextAndIconRect(style, text, icon, font, usedIconSize);
                    textAndIconRect.moveTo(
                        buttonRect.left() + (buttonRect.width() - textAndIconRect.width()) / 2,
                        buttonRect.top() + (buttonRect.height() - textAndIconRect.height()) / 2);
                     drawButtonIcon(style, p, textAndIconRect, icon, usedIconSize);

                     buttonRect.translate(usedIconSize.width(), 0);
                     drawButtonText(style, p, buttonRect, text, style->horizontalTextAlign() | style->verticalTextAlign(), font);

            //merge the alignment of icon and text
            // if the button is aligned horizontaly to the text, text must be drawn with inverted
            // icon's horizontal align and horizontal align of the text
//            if (hasVerticalAlignment(style->iconAlign())) {
//                drawButtonText(style, p, buttonRect, text,
//                               invertAlignment(verticalAlignment(style->iconAlign())) | style->horizontalTextAlign(), font);
//            } else if (hasHorizontalAlignment(style->iconAlign())) {
//                drawButtonText(style, p, buttonRect, text,
//                               invertAlignment(horizontalAlignment(style->iconAlign())) | style->verticalTextAlign(), font);
//            } else {
//                mDebug("PlainQt Style") << "Button has no text align";
//            }
        }
        } else { //Text only
            drawButtonText(style, p, buttonRect, text, style->horizontalTextAlign() | style->verticalTextAlign(), font);
        }
    }
}

void QtMaemo6StylePrivate::drawComboBoxButton(QPainter *p,
                                              const MComboBoxStyle *cbStyle,
                                              const MListItemStyle *liStyle,
                                              const QString &currentText,
                                              const QRect &rect,
                                              const QStyleOption *option,
                                              const Qt::LayoutDirection layoutDirection) const
{

    Q_Q(const QtMaemo6Style);
    if (!cbStyle || !liStyle)
        return;

    q->drawBackground(p, option, rect, liStyle);

    //ToDo: Set appropriate padding
    int paddingLeft, paddingTop, paddingRight, paddingBottom;
    paddingFromStyle(liStyle, &paddingLeft, &paddingTop, &paddingRight, &paddingBottom);

    //draw indicator Image
    QPixmap indicator;
    QString indicatorImageName = cbStyle->indicatorImage();
    QString cacheKey = QString("%1_%2_%3_%4_%5x%6").arg(reinterpret_cast<quintptr>(p->device()))
                       .arg(indicatorImageName)
                       .arg(option ? QtMaemo6StylePrivate::modeFromState(option->state) : "noMode")
                       .arg("ind")
                       .arg(rect.width())
                       .arg(rect.height());

    if (!QPixmapCache::find(cacheKey, indicator) && rect.isValid()) {

        QPixmap *pm = MTheme::pixmapCopy(cbStyle->indicatorImage());
        if (pm) {

            indicator = *pm;

            if (indicator.size().width() > rect.width()
                || indicator.size().height() > rect.height()) {
                indicator = indicator.scaled(rect.size(), Qt::KeepAspectRatio);
            }

            QPixmapCache::insert(cacheKey, indicator);

        }

    }

    // there are no padding values for the indicator, so the spacing has to be taken from the MListItemStyle
    // maybe there is a better way?
    QRect usedIndicatorRect;
    if (layoutDirection == Qt::LeftToRight)
        usedIndicatorRect = rect.adjusted(0, 0, -paddingRight, 0);
    else
        usedIndicatorRect = rect.adjusted(paddingRight, 0, 0, 0);

    q->drawItemPixmap(p, usedIndicatorRect, Qt::AlignVCenter | Qt::AlignRight, indicator);


    //draw text label
    int realPaddingRight = indicator.size().isValid() ? paddingRight+indicator.size().width() : paddingRight;
    QRect textRect;
    if (layoutDirection == Qt::LeftToRight)
        textRect = rect.adjusted(paddingLeft, paddingTop, -realPaddingRight, paddingBottom);
    else
        textRect = rect.adjusted(realPaddingRight, paddingTop, -paddingLeft, paddingBottom);

    const MLabelStyle *textLabelStyle =
            static_cast<const MLabelStyle *>(QtMaemo6StylePrivate::mStyle(
                    QStyle::State_Enabled, "MLabelStyle", cbStyle->titleObjectName()));
    drawLabel(p, textLabelStyle, textRect, currentText);

    //draw information label
    const MLabelStyle *infoLabelStyle =
            static_cast<const MLabelStyle *>(QtMaemo6StylePrivate::mStyle(
                    QStyle::State_Enabled, "MLabelStyle", cbStyle->subTitleObjectName()));
    int hOff = infoLabelStyle ? QFontMetrics(infoLabelStyle->font()).height() : QFontMetrics(qApp->font()).height() ;
    textRect.adjust(0, hOff + 0.25 * hOff, 0, 0);
    drawLabel(p, infoLabelStyle, textRect, qtTrId("qtn_comm_popup_default"));


}

void  QtMaemo6StylePrivate::drawLabel(QPainter *p,
                                      const MLabelStyle *labelStyle,
                                      const QRect &textRect,
                                      const QString &text,
                                      const Qt::Alignment alignment) const
{

    if (text.isEmpty())
        return;

    QColor textColor;
    QFont textFont;

    if (labelStyle) {
        textColor = labelStyle->color();
        textFont = labelStyle->font();
    } else {
        textColor = qApp->palette().text().color();
        textFont =  qApp->font();
    }

    p->save();
    p->setPen(textColor);
    p->setFont(textFont);
    p->drawText(textRect, alignment | Qt::TextHideMnemonic, text);
    p->restore();

}

void QtMaemo6StylePrivate::drawButtonText(const MButtonStyle *style,
        QPainter *painter,
        const QRectF &textRect,
        const QString &text,
        Qt::Alignment align,
        const QFont &font) const
{
    if (text.isEmpty() || !style)
        return;

    // update text area by margin
    int marginLeft = style->textMarginLeft();
    int marginRight = style->textMarginRight();
    int marginTop = style->textMarginTop();
    int marginBottom = style->textMarginBottom();

    QRectF rect;
    rect.setX(textRect.x() + marginLeft);
    rect.setY(textRect.y() + marginTop);
    rect.setWidth(textRect.width() - marginLeft - marginRight);
    rect.setHeight(textRect.height() - marginTop - marginBottom);

    int alignment = align | Qt::TextHideMnemonic;

    painter->setFont(font);
    painter->setPen(style->textColor());
    painter->drawText(rect, alignment  | Qt::TextHideMnemonic, text);

}

void QtMaemo6StylePrivate::drawButtonText(const MButtonStyle *style,
        QPainter *painter,
        const QRectF &textRect,
        const QString &text,
        Qt::Alignment align) const
{
    drawButtonText(style, painter, textRect, text, align, style->font());
}

void QtMaemo6StylePrivate::drawButtonIcon(const MButtonStyle *style,
                                          QPainter *painter,
                                          const QRect &contentsRect,
                                          const QIcon &icon,
                                          const QSize &iconSize, /* = QSize()*/
                                          const bool centerAlways) const
{
    if(style) {
        Q_Q(const QtMaemo6Style);
        QSize usedIconSize = iconSize.isValid() ? iconSize : style->iconSize();

        if (centerAlways) {
             q->drawItemPixmap(painter, contentsRect, Qt::AlignHCenter | Qt::AlignVCenter, icon.pixmap(usedIconSize));
        } else {

            //If alignment is only horizontal oder vertical, center in the other direction
            Qt::Alignment usedAlign = style->iconAlign();
            if (!hasHorizontalAlignment(usedAlign))
                usedAlign |= Qt::AlignHCenter;
            if (!hasVerticalAlignment(usedAlign))
                usedAlign |= Qt::AlignVCenter;

            //        int paddingLeft, paddingTop, paddingRight, paddingBottom;
            //        paddingFromStyle(style, &paddingLeft, &paddingTop, &paddingRight, &paddingBottom);
            //
            //        QRect ctRect = contentsRect.translated(paddingLeft, paddingTop);
            //        ctRect.setWidth(ctRect.width() - (paddingLeft + paddingRight));
            //        ctRect.setHeight(ctRect.height() - (paddingTop + paddingBottom));
            q->drawItemPixmap(painter, contentsRect,usedAlign, icon.pixmap(usedIconSize));
        }
    }
}

QRect QtMaemo6StylePrivate::getTextAndIconRect(const MButtonStyle *style,
        const QString &text,
        const QIcon &icon /* = QIcon()*/,
        const QFont &font /* = QFont()*/,
        const QSize &iconSize /* = QSize()*/) const
{
    QRect textAndIconRect;
    QRect textRect = textBoundingRect(text, font);
    if (!text.isEmpty()) {
        textRect.setWidth(textRect.width() + style->textMarginLeft() + style->textMarginRight());
        textRect.setHeight(textRect.height() + style->textMarginTop() + style->textMarginBottom());
    }
    QSize usedIconSize = iconSize.isValid() ? iconSize : style->iconSize();

    if (hasHorizontalAlignment(style->iconAlign())) {
        if (!icon.isNull()) {
            QRect iconRect = QRect(QPoint(0, 0), usedIconSize);
            //TODO: currently the textMarginLeft is used as space between icon and text
            textAndIconRect = iconRect | textRect.translated(iconRect.width(), 0);
            //if the alignment is left -> calc with textMarginLeft
            //if the alignment is right -> calc with textMarginRight
        } else {
            textAndIconRect = textRect;
        }
    } else {
        if (!icon.isNull()) {
            QRect iconRect = QRect(QPoint(0, 0), usedIconSize);
            //TODO: currently the textMarginLeft is used as space between icon and text
            textAndIconRect = iconRect | textRect.translated(0, iconRect.height());
            //if the alignment is top -> calc with textMarginTop
            //if the alignment is bottom -> calc with textMarginBottom
        } else {
            textAndIconRect = textRect;
        }
    }

//    int paddingLeft, paddingTop, paddingRight, paddingBottom;
//    paddingFromStyle(style, &paddingLeft, &paddingTop, &paddingRight, &paddingBottom);
//
//    //add margins to the final rect
//    textAndIconRect = QRect(textAndIconRect.x(), textAndIconRect.y(),
//                            textAndIconRect.width() + paddingLeft + paddingRight,
//                            textAndIconRect.height() + paddingTop + paddingBottom);

    return textAndIconRect;
}

QRect QtMaemo6StylePrivate::textBoundingRect(const QString &text, const QFont &font) const
{
    QRect rect = QFontMetrics(font).boundingRect(text);
    return QRect(QPoint(0, 0), QSize(rect.x() + rect.width(), rect.height()));
}

void QtMaemo6StylePrivate::paddingFromStyle(const MWidgetStyle *style,
        int *left,
        int *top,
        int *right,
        int *bottom) const
{
    if(style && style->backgroundImage()) {
        style->backgroundImage()->borders(left, right, top, bottom);
        if (left && style->paddingLeft() > *left)
            *left = style->paddingLeft();
        if (top && style->paddingTop() > *top)
            *top = style->paddingTop();
        if (right && style->paddingRight() > *right)
            *right = style->paddingRight();
        if (bottom && style->paddingBottom() > *bottom)
            *bottom = style->paddingBottom();
    } else {
        //if style is invalid or has no backgroundimage, return default padding 0
        *left = *top = *right = *bottom = 0;
    }
}

QSize QtMaemo6StylePrivate::heuristicSizeFromStyle(const MWidgetStyle *style) const
{
    QSize os;
    if (style) {
        os = style->preferredSize();
    }
    return os;
}

void QtMaemo6StylePrivate::setPaletteTextColor(QWidget *widget) const
{

    if (QLineEdit *le = qobject_cast<QLineEdit *>(widget)) {
        const MTextEditStyle *tes =
                static_cast<const MTextEditStyle *>(QtMaemo6StylePrivate::mStyle(
                        le->hasFocus() ? QStyle::State_Enabled | QStyle::State_Selected:  QStyle::State_None,
                        "MTextEditStyle"));

        if (tes) {
            QPalette pal = le->palette();
            pal.setColor(QPalette::Active, QPalette::Text, tes->textColor());
            le->setPalette(pal);
        }
    }
}

QRect QtMaemo6StylePrivate::scrollBarSliderRect(const QStyleOptionComplex *option,
        const QWidget *widget /* = 0*/) const
{
    Q_Q(const QtMaemo6Style);
    if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
        int sliderMaxLength = (scrollBar->orientation == Qt::Horizontal) ?
                              scrollBar->rect.width() : scrollBar->rect.height();
        int sliderMinLength = q->proxy()->pixelMetric(QStyle::PM_ScrollBarSliderMin, scrollBar, widget);
        int sliderLength;

        // calculate slider length
        if (scrollBar->maximum != scrollBar->minimum) {
            uint valueRange = scrollBar->maximum - scrollBar->minimum;
            sliderLength = (scrollBar->pageStep * sliderMaxLength) / (valueRange + scrollBar->pageStep);

            if (sliderLength < sliderMinLength || valueRange > INT_MAX / 2)
                sliderLength = sliderMinLength;
            if (sliderLength > sliderMaxLength)
                sliderLength = sliderMaxLength;
        } else {
            sliderLength = sliderMaxLength;
        }

        int sliderStart = q->sliderPositionFromValue(scrollBar->minimum,
                          scrollBar->maximum,
                          scrollBar->sliderPosition,
                          sliderMaxLength - sliderLength,
                          scrollBar->upsideDown);

        QRect scrollBarRect = scrollBar->rect;
        QRect rect;
        if (scrollBar->orientation == Qt::Horizontal) {
            rect.setRect(sliderStart, 0, sliderLength, scrollBarRect.height());
        } else {
            rect.setRect(0, sliderStart, scrollBarRect.width(), sliderLength);
        }

        return q->visualRect(scrollBar->direction, scrollBarRect, rect);
    }
    return QRect();
}

QPixmap QtMaemo6StylePrivate::borderCroppedPixmap(const MScalableImage* image,
                                                  QSize size,
                                                  int borders,
                                                  int borderLines) const
{
    QPixmap finalPix;
    quintptr pImage = reinterpret_cast<quintptr>(image);
    QString cacheKey = QString("%1_%2_%3_%4_%5").arg(pImage).arg(size.width()).arg(size.height()).arg(borders).arg(borderLines);
    if(QPixmapCache::find(cacheKey, finalPix))
        return finalPix;

    int usedLeftBorder, usedTopBorder, usedRightBorder, usedBottomBorder;
    image->borders(&usedLeftBorder, &usedTopBorder, &usedRightBorder, &usedBottomBorder);

    if(!(borders & topBorder))
        usedTopBorder = 0;
    if(!(borders & rightBorder))
        usedRightBorder = 0;
    if(!(borders & bottomBorder))
        usedBottomBorder = 0;
    if(!(borders & leftBorder))
        usedLeftBorder = 0;

    QSize tempPixmapSize = size;

    QPixmap pix(tempPixmapSize + QSize(usedLeftBorder + usedRightBorder, usedTopBorder + usedBottomBorder));
    pix.fill(Qt::transparent);
    QPainter pixPainter(&pix);
    image->draw(QRect(QPoint(0,0), pix.size()), &pixPainter);

    //take the border color from the outermost top centered pixel
    QColor borderColor = pix.copy(QRect(pix.size().width() / 2, 0, 1, 1)).toImage().pixel(0,0);

    //cut away borders
    finalPix = pix.copy(QRect(QPoint(usedLeftBorder,usedTopBorder), size));

    //draw the closing lines
    QPainter finalPainter(&finalPix);
    finalPainter.setPen(QPen(borderColor, 2));
    if(borderLines & leftBorder)
        finalPainter.drawLine(QPoint(1, 0), QPoint(1, finalPix.height()));
    if(borderLines & topBorder)
        finalPainter.drawLine(QPoint(0, 1), QPoint(finalPix.width(), 1));
    if(borderLines & rightBorder)
        finalPainter.drawLine(QPoint(finalPix.width()-1, 0), QPoint(finalPix.width()-1, finalPix.height()));
    if(borderLines & bottomBorder)
        finalPainter.drawLine(QPoint(0, finalPix.height()-1), QPoint(finalPix.width(), finalPix.height()-1));

    QPixmapCache::insert(cacheKey, finalPix);
    return finalPix;
}

Qt::Alignment QtMaemo6StylePrivate::invertAlignment(Qt::Alignment align) const
{
    Qt::Alignment retAlign;
    //invert horizontal alignment
    if (align.testFlag(Qt::AlignLeft))
        retAlign = Qt::AlignRight;
    else if (align.testFlag(Qt::AlignRight))
        retAlign = Qt::AlignLeft;
    else if (align.testFlag(Qt::AlignHCenter))
        retAlign = Qt::AlignHCenter;

    //invert vertical alignment
    if (align.testFlag(Qt::AlignTop))
        retAlign |= Qt::AlignBottom;
    else if (align.testFlag(Qt::AlignBottom))
        retAlign = Qt::AlignTop;
    else if (align.testFlag(Qt::AlignVCenter))
        retAlign = Qt::AlignVCenter;

    return retAlign;
}

bool QtMaemo6StylePrivate::hasMWindowParent(const QWidget* w) const {
    QWidget* parent = w->parentWidget();
    while(qobject_cast<MWindow*>(parent))
        parent = parent->parentWidget();
    return parent != NULL;
}

QtMaemo6Style::QtMaemo6Style(QtMaemo6StylePrivate &dd)
  : QPlastiqueStyle(),
    d_ptr(& dd) // this is a special case, since we start our shared d-pointer hierarchy within m right here
{
    Q_D(QtMaemo6Style);
    d->q_ptr = this;
}

QtMaemo6Style::QtMaemo6Style()
  : QPlastiqueStyle(),
    d_ptr( new QtMaemo6StylePrivate() )
{
    Q_D(QtMaemo6Style);
    d->q_ptr = this;

    init();
}

QtMaemo6Style::~QtMaemo6Style()
{
    delete d_ptr;
}

void QtMaemo6Style::init()
{
//    QPixmapCache::setCacheLimit( 1000 );
}

bool QtMaemo6Style::isStyled( const QWidget * widget ) const {
    return ! ( (widget &&
                widget->property(M::NoMStyle).isValid()) ||
                qApp->property(M::NoMStyle).isValid() );
}

bool QtMaemo6Style::isOrientationChangeEnabled() const {
#ifdef HAVE_CONTEXTSUBSCRIBER
    Q_D(const QtMaemo6Style);
    return d->m_isOrientationChangeEnabled;
#else
    return false;
#endif
}

void QtMaemo6Style::setOrientationChangeEnabled(bool b) {
    Q_D(QtMaemo6Style);
    d->m_isOrientationChangeEnabled = b;

    //only subscribe if it is really needed
#ifdef HAVE_CONTEXTSUBSCRIBER
    if(b) {
        d->m_orientation.waitForSubscription();
        disconnect(&d->m_orientation);
        if(!connect(&d->m_orientation, SIGNAL(valueChanged()), this, SLOT(doOrientationChange())))
            qCritical() << "Can't connect to orientation change signal";
    }
#endif //HAVE_CONTEXTSUBSCRIBER
}

M::OrientationAngle QtMaemo6Style::orientation() {
    M::OrientationAngle angle;
    QString edge;
#ifdef HAVE_CONTEXTSUBSCRIBER
    Q_D(QtMaemo6Style);
    edge = d->m_orientation.value().toString();
#endif //HAVE_CONTEXTSUBSCRIBER

    if (edge == "top") {
        angle = M::Angle0;
    } else if (edge == "left") {
        angle = M::Angle270;
    } else if (edge == "right") {
        angle = M::Angle90;
    } else if (edge == "bottom") {
        angle = M::Angle180;
    } else {
        angle = M::Angle0;
    }
    return angle;
}

void QtMaemo6Style::orientation(int* o) {
    *o = static_cast<int>(orientation());
}

void QtMaemo6Style::polish(QApplication *app)
{
    QPlastiqueStyle::polish(app);
}

void QtMaemo6Style::polish(QWidget *widget)
{
    Q_D(QtMaemo6Style);

    if(!widget)
        return;

    if(qobject_cast<QDesktopWidget*>(widget))
        return;

    //Qt internal class, skip this
    if(d->m_excludeClasses.contains(widget->metaObject()->className()))
        return;

    if( !isStyled( widget ) ) {
        return;
    }

    if(qobject_cast<MWindow*>(widget)) {
        return;
    }

    /*
    //skip the scrollbar container widgets of QAbstractScrollAreas
    if( (widget->objectName().contains("qt_scrollarea_hcontainer")
        || widget->objectName().contains("qt_scrollarea_vcontainer")
        || widget->objectName().contains("qt_scrollarea_viewport"))
        && qobject_cast<MWindow*>(widget->parentWidget()))
        return;
    */

    //skip the viewports, they are done by the scrollarea
    if( QAbstractScrollArea* sa = qobject_cast<QAbstractScrollArea*>(widget->parentWidget())) {
        if(sa->viewport() == widget) //it's the viewport
            return;
    }

#ifdef M_LOG_POLISH
    QString filename = QString("/tmp/mstyle_%1.log").arg( QCoreApplication::applicationFilePath().section('/', -1 ) );

    QFile file( filename );
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << filename << " polish: " << (int)(widget) << " " <<  widget->metaObject()->className()
                        << ' ' << widget->objectName() << "\n";
        file.close();
    }
    else {
        mDebug("PlainQt Style") << "unable to open" << filename;
    }
#endif

    // Lazy initialization of the MFramework.
    // This is needed to guarantee that actual MApplications will work as well.
    if (!d->m_isMInitialized) {
        d->initM();
    }

     if((qobject_cast<QToolBox *>(widget)) || qobject_cast<QTabBar *>(widget))
         widget->setAutoFillBackground(true);

    if(QMainWindow* mw = qobject_cast<QMainWindow*>(widget)) {
        mw->setFixedSize(MDeviceProfile::instance()->resolution());
        setPaletteBackground(mw);

        QtMaemo6ApplicationWindow* aw = QtMaemo6ApplicationWindow::createInstance(mw);

        aw->hideMenu();
    }



    //Ensure that the widget draws its background transparent
    if (   !qobject_cast<QMenu *>(widget)
        && !qobject_cast<QMainWindow*>(widget)) {
         if(!widget->testAttribute(Qt::WA_TranslucentBackground) && widget->parent()
             && widget->palette().color(QPalette::Window) == qApp->palette().color(QPalette::Window)) {
            //FIXME: public API usage
            QPalette pal = widget->palette();
            pal.setBrush(QPalette::Window, Qt::transparent);
            widget->setPalette(pal);
        }
    }

    if (QtMaemo6TitleBar *titleBar = qobject_cast<QtMaemo6TitleBar *>(widget)) {
            // apply properties of the navigation bar style
            const MNavigationBarStyle *style =
                static_cast<const MNavigationBarStyle *>(QtMaemo6StylePrivate::mStyle(QStyle::State_Active,
                        "MNavigationBarStyle"));
            /*
            const MLabelStyle *menuStyle =
                static_cast<const MLabelStyle *>(QtMaemo6StylePrivate::mStyle(QStyle::State_None,
                        "MLabelStyle", "NavigationBarMenuButtonLabel"));
            Q_UNUSED( menuStyle );
            */
            if(style) {
                titleBar->setItemSpacing( style->itemSpacing() );
                titleBar->setFixedHeight( style->minimumSize().height() );
            }
    }


#ifdef MOVE_ACTIONS_FROM_TOOLBAR_TO_TITLEBAR
    if (QtMaemo6TitleBar *titleBar = qobject_cast<QtMaemo6TitleBar *>(widget)) {
        foreach(QAction * action, d->m_toolBarActions) { // krazy:exclude=foreach
            titleBar->addAction(action);
        }
    }

    QToolBar *toolBar = qobject_cast<QToolBar *>(widget);
    if (toolBar && qobject_cast<QMainWindow *>(widget->parent())) {
        foreach(QAction * action, toolBar->actions()) { // krazy:exclude=foreach
            if (d->m_toolBarActions.count() < d->m_actionsInTitleBarCount) {
                d->m_toolBarActions.append(action);
                toolBar->removeAction(action);
            } else
                break;
        }
        if (toolBar->actions().isEmpty()) {
            QMainWindow *mw = qobject_cast<QMainWindow *>(widget->parent());
            mw->removeToolBar(toolBar);
        }
    }
#endif

    if (qobject_cast<QScrollBar *>(widget)) {
        widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
        d->m_scrollBarEventFilter->enableOn(widget);
    }

    if (QAbstractScrollArea *abstractScrollArea = qobject_cast<QAbstractScrollArea *>(widget)) {
        d->m_scrollBarEventFilter->enableOn(widget);
        //must use inherits() because it's an private Qt class
        if (!abstractScrollArea->inherits("QComboBoxListView")) {
            //don't do this for QComboBoxListView, because it uses the QItemDelegate
            //which not uses the style
            QPalette pal = abstractScrollArea->viewport()->palette();
            pal.setBrush(QPalette::Base, Qt::transparent);
            abstractScrollArea->viewport()->setPalette(pal);
        } else {
            //this must be set for ComboBox DropDowns, don't know why!
            QPalette pal = abstractScrollArea->viewport()->palette();
            pal.setBrush(QPalette::Base, Qt::white);
            abstractScrollArea->viewport()->setPalette(pal);
        }

        foreach(QObject *child, abstractScrollArea->children()) {
            if (child->isWidgetType()) {
              QtScroller::grabGesture(child, QtScroller::TouchGesture);
              QtScroller::grabGesture(child, QtScroller::LeftMouseButtonGesture);
              child->installEventFilter(d->m_windowEventFilter);
          }
        }

    }


    if (QStatusBar *statusBar = qobject_cast<QStatusBar *>(widget)) {
        //only use statusbars directly from MainWindow
        if (qobject_cast<QMainWindow *>(statusBar->parent())) {
            //FIXME: add handling here for not showing empty statusbars
            d->m_statusBar = statusBar;
        }
        //FIXME: Public API usage
        //FIXME: Figure out why this triggers a crash with Qt 4.7
//        statusBar->setSizeGripEnabled(false);
    }

    if (QMenuBar *menuBar = qobject_cast<QMenuBar *>(widget)) {
        //Only take the menubar from the main window
        //TODO: this doesn't work for applications that use shared menuBars, like eg. some
        //      mac applications do.
        if (menuBar->parent()) {
            if (qobject_cast<QMainWindow *>(menuBar->parent())) {
                d->m_menuBar = menuBar;
                //do not remove the menuBar from the mainWindow, it may be changed
                //by the user
            }
        }
    }

    if(QComboBox* comboBox = qobject_cast<QComboBox*>(widget)) {
        if(comboBox->isEditable()) {
            comboBox->lineEdit()->setVisible(false);
        }
    }
    if(QHeaderView* hView = qobject_cast<QHeaderView*>(widget)) {
        hView->viewport()->setBackgroundRole(QPalette::Window);
    }

    if(QSlider* slider = qobject_cast<QSlider*>(widget)) {
        QtMaemo6SliderPopUp* sliderPopup = new QtMaemo6SliderPopUp();
        sliderPopup->enableOn(slider);
	const MLabelStyle *labelTitle =
	    static_cast<const MLabelStyle *>(QtMaemo6StylePrivate::mStyle(QStyle::State_Active,
					      "MLabelStyle", "MSliderHandleLabel"));
	QPalette pal = sliderPopup->palette();
	pal.setColor(QPalette::WindowText, labelTitle->color());
	sliderPopup->setPalette(pal);
    }
    if(QLineEdit* le = qobject_cast<QLineEdit*>(widget)) {
        const MTextEditStyle *styleActive =
                static_cast<const MTextEditStyle *>(QtMaemo6StylePrivate::mStyle(QStyle::State_Enabled,
                        "MTextEditStyle"));
        if(styleActive) {
            QPalette pal = le->palette();
            pal.setColor(QPalette::Active, QPalette::Text, styleActive->textColor());
            pal.setColor(QPalette::Active, QPalette::Highlight, styleActive->selectionBackgroundColor());
            pal.setColor(QPalette::Active, QPalette::HighlightedText, styleActive->selectionTextColor());
            le->setPalette(pal);
        }
    }
    if(QLabel* label = qobject_cast<QLabel*>(widget)) {

        //style for slider popups is set above
        if (!label->objectName().endsWith(QLatin1String("SliderPopUp"))) {

            const MLabelStyle *styleActive =
                    static_cast<const MLabelStyle *>(QtMaemo6StylePrivate::mStyle(QStyle::State_Active,
                                                                                  "MLabelStyle"));
            const MLabelStyle *styleInactive =
                    static_cast<const MLabelStyle *>(QtMaemo6StylePrivate::mStyle(QStyle::State_None,
                                                                                  "MLabelStyle"));
            if(styleActive && styleInactive) {
                QPalette pal = label->palette();
                pal.setColor(QPalette::Inactive, QPalette::Foreground, styleInactive->color());
                pal.setColor(QPalette::Active, QPalette::Foreground, styleActive->color());
                label->setPalette(pal);
                label->setFont(styleActive->font());
            }
        }
    }

    if(QAbstractItemView* aiv = qobject_cast<QAbstractItemView *>(widget)) {
        qWarning() << "is QAbstractItemView";
        aiv->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        aiv->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    }

    widget->installEventFilter(d->m_windowEventFilter);
}

void QtMaemo6Style::drawPrimitive(PrimitiveElement element,
                                  const QStyleOption *option,
                                  QPainter *painter,
                                  const QWidget *widget) const
{
    if( !isStyled( widget ) ) {
        QPlastiqueStyle::drawPrimitive(element, option, painter, widget);
        return;
    }

    // Handle rare case where this method is invoked before the polish is invoked
    Q_D(const QtMaemo6Style);
    if (!d->m_isMInitialized) {
        mDebug("PlainQt Style") << Q_FUNC_INFO << "entered before initialization of MComponentData";
        return;
    }

    switch (element) {
    case PE_Widget: {
        if(widget->parentWidget() == 0) {
            //draw the MApplicationPage background if the widget has no parent
            const MApplicationPageStyle *style =
                static_cast<const MApplicationPageStyle *>(
                    QtMaemo6StylePrivate::mStyle(option->state, "MApplicationPageStyle"));
            if(style)
                d->drawScalableImage(painter, option, widget->rect(), style->backgroundImage(), style, widget, "canvas-image");
        }
    }
    break;

    case PE_PanelLineEdit: {
        // don't draw the line edit, if it is part of a spinbox or a combobox
        if (widget && (qobject_cast<QAbstractSpinBox *>(widget->parent()) || qobject_cast<QComboBox*>(widget->parent()))) {
            mDebug("PlainQt Style") << "### skipping LineEdit";
            break;
        }

        if (widget) {
            if (const QStyleOptionFrame *panel = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
                const MTextEditStyle *style =
                        static_cast<const MTextEditStyle *>(QtMaemo6StylePrivate::mStyle(panel->state,
                                                                                         "MTextEditStyle", "", "", widget->hasFocus()));
                if(style) {
                    drawBackground(painter, option, panel->rect, style, widget);
                } else {
                    qCritical() << "Could not load MTextEditStyle for QLineEdit";
                }
            }
        }
    }
    break;
    case PE_PanelItemViewItem: { //draw the background of ItemViewItems
            if (const QStyleOptionViewItemV4 *item = qstyleoption_cast<const QStyleOptionViewItemV4 *>(option)) {
                const MContentItemStyle *style =
                        static_cast<const MContentItemStyle *>(QtMaemo6StylePrivate::mStyle(item->state,
                                                                                            "MContentItemStyle"));
                const QAbstractItemModel *model = item->index.model();

                if (model) {
                    //it's a simple Listview
                    if (model->columnCount() == 1) {
                        if (model->rowCount() == 1) {
                            d->drawScalableImage(painter, option, item->rect, style->backgroundImageSingle(), style);
                        } else if (item->index.row() == 0) {
                            d->drawScalableImage(painter, option, item->rect, style->backgroundImageSinglecolumnTop(), style);
                        } else if (item->index.row() == model->rowCount() - 1) {
                            d->drawScalableImage(painter, option, item->rect, style->backgroundImageSinglecolumnBottom(), style);
                        } else {
                            d->drawScalableImage(painter, option, item->rect, style->backgroundImageSinglecolumnCenter(), style);
                        }
                    } else {
                        //TODO: Add implementation for tableviews here
                        QPlastiqueStyle::drawPrimitive(element, option, painter, widget);
                        if (model->rowCount() == 1) {
                            if (item->index.column() == 0) {
                                d->drawScalableImage(painter, option, item->rect, style->backgroundImageSinglerowLeft(), style);
                            } else if (item->index.column() == model->columnCount() - 1) {
                                d->drawScalableImage(painter, option, item->rect, style->backgroundImageSinglerowRight(), style);
                            } else {
                                d->drawScalableImage(painter, option, item->rect, style->backgroundImageSinglerowCenter(), style);
                            }
                        } else if (item->index.row() == 0) {
                            if (item->index.column() == 0) {
                                d->drawScalableImage(painter, option, item->rect, style->backgroundImageTopLeft(), style);
                            } else if (item->index.column() == model->columnCount() - 1) {
                                d->drawScalableImage(painter, option, item->rect, style->backgroundImageTopRight(), style);
                            } else {
                                d->drawScalableImage(painter, option, item->rect, style->backgroundImageTop(), style);
                            }
                        } else if (item->index.row() == model->rowCount() - 1) {
                            if (item->index.column() == 0) {
                                d->drawScalableImage(painter, option, item->rect, style->backgroundImageBottomLeft(), style);
                            } else if (item->index.column() == model->columnCount() - 1) {
                                d->drawScalableImage(painter, option, item->rect, style->backgroundImageBottomRight(), style);
                            } else {
                                d->drawScalableImage(painter, option, item->rect, style->backgroundImageBottom(), style);
                            }
                        } else {
                            if (item->index.column() == 0) {
                                d->drawScalableImage(painter, option, item->rect, style->backgroundImageLeft(), style);
                            } else if (item->index.column() == model->columnCount() - 1) {
                                d->drawScalableImage(painter, option, item->rect, style->backgroundImageRight(), style);
                            } else {
                                d->drawScalableImage(painter, option, item->rect, style->backgroundImageCenter(), style);
                            }
                        }
                    }
                }

            }
        }
        break;
    case PE_IndicatorCheckBox: {
         if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            d->drawCheckBox(painter, QString(), btn->icon, btn->rect, option);
         }
    }
    break;
    case PE_IndicatorBranch: {
        QPlastiqueStyle::drawPrimitive(element, option, painter, widget);
        if (option->state.testFlag(QStyle::State_Children)) {
            QToolButton button;
            QStyleOptionToolButton buttonOption;
            buttonOption.initFrom(&button);
            buttonOption.rect = option->rect;
            if (option->state.testFlag(QStyle::State_Open)) {
                buttonOption.text = '-';
            } else {
                buttonOption.text = '+';
            }

            QSize size = sizeFromContents(QStyle::CT_ToolButton, &buttonOption, QSize(), &button);

            QRect buttonRect = QRect(option->rect.left() + (option->rect.width() - size.width()) / 2,
                                     option->rect.top() + (option->rect.height() - size.height()) / 2,
                                     size.width(), size.height());
            buttonOption.rect = buttonRect;

            drawComplexControl(QStyle::CC_ToolButton, &buttonOption, painter, &button);
        }
    }
    break;
    case PE_Frame: {
        //don't draw the frame for item views, this is done by the items themselfs
        if (qobject_cast<const QAbstractItemView *>(widget)) {
        } else
            QPlastiqueStyle::drawPrimitive(element, option, painter, widget);
    }
    break;
    case PE_FrameFocusRect: {
        //The focus rect of selected items in AbstractItemViews
        //don't draw anything here
    }
    default: {
        QPlastiqueStyle::drawPrimitive(element, option, painter, widget);
    }
    break;
    }

}

void QtMaemo6Style::drawControl(ControlElement element,
                                const QStyleOption *opt,
                                QPainter *p,
                                const QWidget *widget) const
{
    if( !isStyled( widget ) ) {
        QPlastiqueStyle::drawControl(element, opt, p, widget);
        return;
    }

    // Handle rare case where this method is invoked before the polish is invoked
    Q_D(const QtMaemo6Style);
    if (!d->m_isMInitialized) {
        mDebug("PlainQt Style") << Q_FUNC_INFO << "entered before initialization of MComponentData";
        return;
    }

    switch (element) {

    case CE_ComboBoxLabel: {
            mDebug("PlainQt Style") << "### Combobox line edit";
    }
    break;

    case CE_PushButton: {
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {

            QStyleOptionButton subopt = *btn;
            subopt.rect = subElementRect(SE_PushButtonContents, btn, widget);

            const QAbstractButton *buttonWidget = qobject_cast<const QAbstractButton * >(widget);
            if (buttonWidget && buttonWidget->isCheckable())
                d->drawToggleButton(p, subopt.text, subopt.icon, subopt.rect, opt, "MButtonStyle");
            else
                d->drawBasicButton(p, subopt.text, subopt.icon, subopt.rect, opt, "MButtonStyle");
        }
    }
    break;

    case CE_ProgressBar: {
        if (const QStyleOptionProgressBar * pb
                = qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {
            QStyleOptionProgressBarV2 subopt = *pb;
            subopt.rect = subElementRect(SE_ProgressBarGroove, pb, widget);
            proxy()->drawControl(CE_ProgressBarGroove, &subopt, p, widget);
            subopt.rect = subElementRect(SE_ProgressBarContents, pb, widget);
            proxy()->drawControl(CE_ProgressBarContents, &subopt, p, widget);
        }
    }
    break;

    case CE_RadioButton:
    case CE_CheckBox: {
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {

            bool isCheckBox = (element == CE_CheckBox);

            QStyleOptionButton subopt = *btn;

            //draw checkboxes for radio buttons
            subopt.rect = subElementRect(SE_CheckBoxIndicator, btn, widget);
            proxy()->drawPrimitive(PE_IndicatorCheckBox, &subopt, p, widget);

            subopt.rect = subElementRect(isCheckBox ? SE_CheckBoxContents
                                         : SE_RadioButtonContents,
                                         btn, widget);

            proxy()->drawControl(isCheckBox ? CE_CheckBoxLabel
                                 : CE_RadioButtonLabel,
                                 &subopt, p, widget);
        }
        break;
    }
    case CE_TabBarTabShape: {
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {

            QStyleOptionButton btn;
            btn.initFrom(widget);
            btn.rect = tab->rect;
            btn.state = tab->state;

            const MButtonStyle *style =
                static_cast<const MButtonStyle *>(QtMaemo6StylePrivate::mStyle(opt->state,
                                                    "MButtonStyle"));
            // Don't show the edge of the button that is closest to the
            // TabBarBase.
            // FIXME: Solve for other tab bar orientations
            if (style && style->backgroundImage()) {
                int left, right, top, bottom;
                style->backgroundImage()->borders(&left, &right, &top, &bottom);

                qreal cutOff = 0;
                qreal origSize = 0;

                const QPixmap *pixmap = style->backgroundImage()->pixmap();
                if (pixmap) {
                    int buttonHeight = btn.rect.height();
                    int buttonWidth = btn.rect.width();

                    switch (tab->shape) {
                    case QTabBar::RoundedNorth:
                    case QTabBar::TriangularNorth:
                        cutOff = bottom;
                        origSize = pixmap->height();
                        btn.rect.setHeight(buttonHeight * origSize / (origSize - cutOff));
                        break;

                    case QTabBar::RoundedSouth:
                    case QTabBar::TriangularSouth:
                        cutOff = top;
                        origSize = pixmap->height();
                        btn.rect.setTop(-buttonHeight * cutOff / origSize);
                        btn.rect.setHeight(buttonHeight * origSize / (origSize - cutOff));
                        break;

                    case QTabBar::RoundedWest:
                    case QTabBar::TriangularWest:
                        cutOff = right;
                        origSize = pixmap->width();
                        btn.rect.setWidth(buttonWidth * origSize / (origSize - cutOff));
                        break;

                    case QTabBar::RoundedEast:
                    case QTabBar::TriangularEast:
                        cutOff = left;
                        origSize = pixmap->width();
                        btn.rect.setLeft(-buttonWidth * cutOff / origSize);
                        btn.rect.setWidth(buttonWidth * origSize / (origSize - cutOff));
                        break;

                    default:
                        break;
                    }
                }
            }

            d->drawBasicButton(p, QString(), QIcon(),
                               btn.rect, &btn, style);
            p->setPen(style->textColor());
            p->drawText(tab->rect, Qt::AlignCenter | Qt::TextHideMnemonic, tab->text);
        }
        break;
    }
    case CE_ToolBoxTabShape: {
            if (const QStyleOptionToolBox *toolBoxTab = qstyleoption_cast<const QStyleOptionToolBox *>(opt)) {
                QStyleOptionTab sot;
                sot.initFrom(widget);
                sot.shape = QTabBar::RoundedNorth;
                sot.position = QStyleOptionTab::OnlyOneTab;
                sot.text = toolBoxTab->text;
                sot.rect = toolBoxTab->rect;
                sot.icon = toolBoxTab->icon;
                sot.state = toolBoxTab->state;
                proxy()->drawControl(CE_TabBarTabShape, &sot, p, widget);
            }
            break;
        }
    case CE_TabBarTabLabel:
    case CE_ToolBoxTabLabel:
        // don't draw the label, it's already handled by CE_TabBarTabShape
        break;
    case CE_ToolBar: {
        //const QToolBar* toolBar = qobject_cast<const QToolBar*>(widget);
        if (widget->actions().count() > 0)
            QPlastiqueStyle::drawControl(element, opt, p, widget);
        break;
    }
    case CE_ProgressBarGroove: {
        if (const QStyleOptionProgressBar *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {
            const MProgressIndicatorStyle *style =
                static_cast<const MProgressIndicatorStyle *>(QtMaemo6StylePrivate::mStyle(bar->state,
                        "MProgressIndicatorStyle", QString(), "bar"));
            if (style && style->progressBarBackground())
                d->drawScalableImage(p, opt, bar->rect, style->progressBarBackground(), style, widget, "groove");
        }
        break;
    }
    case CE_ProgressBarContents: {

        if (const QStyleOptionProgressBar *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {
            const MProgressIndicatorStyle *style =
                static_cast<const MProgressIndicatorStyle *>(QtMaemo6StylePrivate::mStyle(bar->state,
                        "MProgressIndicatorStyle", QString(), "bar"));
            if (style && style->knownBarTexture()) {

                bool reverse = qApp->isRightToLeft();

                QRect r(bar->rect);

                int left, right;
                style->knownBarTexture()->borders(&left, &right, NULL, NULL);

                qreal position = (qreal)(bar->progress - bar->minimum) / (qreal)(bar->maximum - bar->minimum);
                qreal distance = position * (qreal) r.width();

                // need to draw in 1 or 2 parts, depending if the indicator element goes across the ends
                // one draw call
                if (!reverse) {
                    r.setLeft(0);
                    r.setRight(distance);
                } else {
                    r.setRight(r.width());
                    r.setLeft(r.width() - distance);
                }
                d->drawScalableImage(p, opt, r, style->knownBarTexture(), style, widget, "contents", false);
            }
        }
        break;
    }
    case CE_ScrollBarAddLine:
    case CE_ScrollBarSubLine:
    case CE_ScrollBarAddPage:
    case CE_ScrollBarSubPage:
    case CE_ScrollBarFirst:
    case CE_ScrollBarLast:
        //QPlastiqueStyle::drawControl(element, opt, p, widget);
        //these cases do not occur
        break;
    case CE_ScrollBarSlider: {
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const MPositionIndicatorStyle *style =
                static_cast<const MPositionIndicatorStyle *>(QtMaemo6StylePrivate::mStyle(slider->state,
                        "MPositionIndicatorStyle"));

            drawBackground(p, opt, slider->rect, style, widget);

            //retrieving the slider size, and position within the groove
            QRect scSliderRect = subControlRect(CC_ScrollBar, slider, SC_ScrollBarSlider, widget);

            d->drawScalableImage(p, opt, scSliderRect, style->indicatorImage(), style, widget, "groove", false);
        }
    }
    break;
    case CE_ItemViewItem: {
        if (const QStyleOptionViewItemV4 *item = qstyleoption_cast<const QStyleOptionViewItemV4 *>(opt)) {
            proxy()->drawPrimitive(PE_PanelItemViewItem, opt, p, widget);
            const MContentItemStyle *style =
                static_cast<const MContentItemStyle *>(QtMaemo6StylePrivate::mStyle(item->state,
                        "MContentItemStyle"));
            if(style) {
                const MLabelStyle *labelTitle =
                    static_cast<const MLabelStyle *>(QtMaemo6StylePrivate::mStyle(item->state,
                                                      "MLabelStyle", style->titleObjectName()));
                //subtitles for list view items are currently not supported by qt.
                //const MLabelStyle* labelSubTitle =
                //        static_cast<const MLabelStyle*>( QtMaemo6StylePrivate::mStyle( item->state,
                //                                                                           "MLabelStyle", style->subtitleObjectName() ) );
                const MImageWidgetStyle *labelIcon =
                    static_cast<const MImageWidgetStyle *>(QtMaemo6StylePrivate::mStyle(item->state,
                            "MImageWidgetStyle", style->imageObjectName()));

                QString itemText = qvariant_cast<QString>(item->index.data(Qt::DisplayRole));
                QIcon itemIcon = qvariant_cast<QIcon>(item->index.data(Qt::DecorationRole));

                QRect rect = item->rect;

                if (!itemIcon.isNull() && labelIcon) {
                    rect.setTop(rect.top() + labelIcon->marginTop());
                    rect.setLeft(rect.left() + labelIcon->marginLeft());
                    p->drawPixmap(rect.topLeft(), itemIcon.pixmap(labelIcon->preferredSize()));
                    rect.setLeft(rect.left() + labelIcon->preferredSize().width() + labelIcon->marginRight());
                }

                if(labelTitle) {
                    rect.setLeft(rect.left() + labelTitle->marginLeft());
                    QFont labelFont = labelTitle->font();

                    p->setFont(labelFont);
                    p->setPen(labelTitle->color());
                    p->drawText(rect, Qt::AlignLeft | Qt::AlignTop | Qt::TextHideMnemonic, itemText);

                }
            }
        }
    }
    break;
    case CE_HeaderSection: {
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
            const MButtonStyle * style =
                static_cast<const MButtonStyle *>( QtMaemo6StylePrivate::mStyle( header->state,
                                                       "MButtonStyle") );

            if(style) {
                int left, top, right, bottom;
                style->backgroundImage()->borders(&left, &top, &right, &bottom);

                p->fillRect(opt->rect, Qt::transparent);

                QtMaemo6StylePrivate::Borders leftBorder = QtMaemo6StylePrivate::leftBorder;
                QtMaemo6StylePrivate::Borders rightBorder = QtMaemo6StylePrivate::rightBorder;

                if(opt->direction == Qt::RightToLeft) {
                    leftBorder = QtMaemo6StylePrivate::rightBorder;
                    rightBorder = QtMaemo6StylePrivate::leftBorder;
                }

                if(header->orientation == Qt::Horizontal) {
                    if(header->position == QStyleOptionHeader::Beginning) {
                        QPixmap pix = d->borderCroppedPixmap(style->backgroundImage(), opt->rect.size(),
                                      QtMaemo6StylePrivate::bottomBorder | rightBorder);
                        p->drawPixmap(opt->rect.topLeft(), pix);
                    } else if(header->position == QStyleOptionHeader::Middle) {
                        QPixmap pix = d->borderCroppedPixmap(style->backgroundImage(), opt->rect.size(),
                                      QtMaemo6StylePrivate::bottomBorder | rightBorder | leftBorder,
                                      QtMaemo6StylePrivate::bottomBorder | rightBorder);
                        p->drawPixmap(opt->rect.topLeft(), pix);
                    } else if(header->position == QStyleOptionHeader::End) {
                        QPixmap pix = d->borderCroppedPixmap(style->backgroundImage(), opt->rect.size(),
                                      QtMaemo6StylePrivate::bottomBorder | leftBorder,
                                      QtMaemo6StylePrivate::bottomBorder);
                        p->drawPixmap(opt->rect.topLeft(), pix);
                    }
                } else if(header->orientation == Qt::Vertical) {
                    if(header->position == QStyleOptionHeader::Beginning) {
                        QPixmap pix = d->borderCroppedPixmap(style->backgroundImage(), opt->rect.size(),
                                      QtMaemo6StylePrivate::bottomBorder | rightBorder);
                        p->drawPixmap(opt->rect.topLeft(), pix);
                    } else if(header->position == QStyleOptionHeader::Middle) {
                        QPixmap pix = d->borderCroppedPixmap(style->backgroundImage(), opt->rect.size(),
                                      QtMaemo6StylePrivate::bottomBorder | QtMaemo6StylePrivate::topBorder | rightBorder,
                                      QtMaemo6StylePrivate::bottomBorder | rightBorder);
                        p->drawPixmap(opt->rect.topLeft(), pix);
                    } else if(header->position == QStyleOptionHeader::End) {
                        QPixmap pix = d->borderCroppedPixmap(style->backgroundImage(), opt->rect.size(),
                                      QtMaemo6StylePrivate::topBorder | rightBorder,
                                      rightBorder);
                        p->drawPixmap(opt->rect.topLeft(), pix);
                    }
                }

                p->setPen(style->textColor());
                p->drawText(header->rect, Qt::AlignCenter | Qt::TextHideMnemonic, header->text);
            }
        }
    }
    break;
    case CE_HeaderLabel:
    // don't draw header labels, it's already done by CE_HeaderSection
    break;
    default: {
        QPlastiqueStyle::drawControl(element, opt, p, widget);
    }
    break;
    }
}

void QtMaemo6Style::drawComplexControl(ComplexControl control,
                                       const QStyleOptionComplex *opt,
                                       QPainter *p,
                                       const QWidget *widget /* = 0*/) const
{
    if( !isStyled( widget ) ) {
        QPlastiqueStyle::drawComplexControl(control, opt, p, widget);
        return;
    }

    // Handle rare case where this method is invoked before the polish is invoked
    Q_D(const QtMaemo6Style);
    if (!d->m_isMInitialized) {
        mDebug("PlainQt Style") << Q_FUNC_INFO << "entered before initialization of MComponentData";
        return;
    }

    switch (control) {
    case CC_ComboBox: {
        if (const QStyleOptionComboBox *btn = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            mDebug("PlainQt Style") << "### drawing ComboBox";
            QStyleOptionComboBox subopt = *btn;
            subopt.rect = subControlRect(control, opt, SC_ToolButton, widget);
            if (d->m_pressedComboBox && d->m_pressedComboBox == widget)
                subopt.state |= QStyle::State_Sunken;

            const MComboBoxStyle *cbStyle =
                static_cast<const MComboBoxStyle *>(QtMaemo6StylePrivate::mStyle(subopt.state, "MComboBoxStyle", "", "button"));
            const MListItemStyle *liStyle =
                  static_cast<const MListItemStyle *>(QtMaemo6StylePrivate::mStyle(subopt.state, "MListItemStyle", "", "button", false, new MComboBox()));

            if (widget)
                d->drawComboBoxButton(p, cbStyle, liStyle, subopt.currentText, subopt.rect, opt, widget->layoutDirection());
            else
                d->drawComboBoxButton(p, cbStyle, liStyle, subopt.currentText, subopt.rect, opt, qApp->layoutDirection());
        }
    }
    break;
    case CC_ToolButton: {
        if (const QStyleOptionToolButton *btn = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) {
            QStyleOptionToolButton subopt = *btn;
            subopt.rect = subControlRect(control, opt, SC_ToolButton, widget);

            const MButtonStyle *style =
                static_cast<const MButtonStyle *>(QtMaemo6StylePrivate::mStyle(subopt.state, "MButtonStyle"));
            const MButtonStyle *styleFont =
                static_cast<const MButtonStyle *>(QtMaemo6StylePrivate::mStyle(subopt.state, "MButtonStyle",
                                                    "NavigationBarToolBarButton"));
            if (subopt.toolButtonStyle == Qt::ToolButtonIconOnly)
                subopt.text.clear();
            d->drawBasicButton(p, subopt.text, subopt.icon, subopt.rect, opt, style, styleFont->font(), styleFont->iconSize());
        }
    }
    break;

    case CC_Slider: {
        p->save();
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const MSliderStyle *style =
                static_cast<const MSliderStyle *>(QtMaemo6StylePrivate::mStyle(slider->state,
                                                    "MSliderStyle"));

            // draw widget background
            drawBackground(p, opt, widget->rect(), style, widget);

            // draw groove
            bool isHorizontal = slider->orientation == Qt::Horizontal;

            QRect grooveArea = proxy()->subControlRect(CC_Slider, opt,
                               SC_SliderGroove,
                               widget);

            int maxSliderLength = proxy()->pixelMetric(PM_SliderLength,
                                  slider, widget);
            int grooveThickness = style->grooveThickness();

            QRect grooveRect;
            if (isHorizontal) {
                grooveRect = grooveArea.adjusted(maxSliderLength / 2,
                                                 (grooveArea.height() - grooveThickness) / 2,
                                                 - maxSliderLength / 2,
                                                 - (grooveArea.height() - grooveThickness) / 2);
            } else {
                grooveRect = grooveArea.adjusted((grooveArea.width() - grooveThickness) / 2,
                                                 maxSliderLength / 2,
                                                 - (grooveArea.width() - grooveThickness) / 2,
                                                 - maxSliderLength / 2);
            }

            QtMaemo6StylePrivate::drawSliderBaseBackground(p, opt, grooveRect, style, maxSliderLength);

            // Handle
            QPixmap handlePixmap;

            if (isHorizontal) {
                const QPixmap *handleSourcePixmap =
                    (slider->state & State_Sunken)
                    ? style->handlePressedPixmap()
                    : style->handlePixmap();
                handlePixmap = *handleSourcePixmap;
            } else {
                const QPixmap *handleSourcePixmap =
                    (slider->state & State_Sunken)
                    ? style->handleVerticalPressedPixmap()
                    : style->handleVerticalPixmap();
                handlePixmap = *handleSourcePixmap;
            }

            QRect handleRect = proxy()->subControlRect(CC_Slider, opt,
                               SC_SliderHandle,
                               widget);
            p->drawPixmap(handleRect, handlePixmap);
        }
        p->restore();
    }
    break;
    case CC_SpinBox: {
        if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(opt)) {
            p->save();

            // Draw the line edit background
            QStyleOptionFrame frameOpt;
            if (QLineEdit *lineedit = qFindChild<QLineEdit *>(widget))
                frameOpt.initFrom(lineedit);

            frameOpt.rect = spinBox->rect;
            frameOpt.state |= QStyle::State_Sunken;

            drawPrimitive(QStyle::PE_PanelLineEdit, &frameOpt, p, widget);

            bool upSunken = (spinBox->activeSubControls & SC_SpinBoxUp) && (spinBox->state & (State_Sunken | State_On));
            bool downSunken = (spinBox->activeSubControls & SC_SpinBoxDown) && (spinBox->state & (State_Sunken | State_On));

            // Rects
            QRect upRect = proxy()->subControlRect(CC_SpinBox, opt, SC_SpinBoxUp, widget);
            QRect downRect = proxy()->subControlRect(CC_SpinBox, opt, SC_SpinBoxDown, widget);
            QRect frameEditRect = proxy()->subControlRect(CC_SpinBox, opt, SC_SpinBoxEditField, widget);

            // Best guess for size of +/- symbols
            int offset = frameEditRect.height() / 6;
            if (spinBox->buttonSymbols != QAbstractSpinBox::NoButtons) {
                QPoint center;
                if (spinBox->subControls & SC_SpinBoxUp) {
                    center = upRect.center();
                    // offset for pressed state
                    if (upSunken) {
                        ++center.rx();
                        ++center.ry();
                    }

                    // Choose correct palette for + symbol
                    QPalette pal = spinBox->palette;
                    if (!(spinBox->stepEnabled & QAbstractSpinBox::StepUpEnabled)) {
                        pal.setCurrentColorGroup(QPalette::Disabled);
                    }
                    // Draw + symbol
                    QBrush indicatorBrush = pal.buttonText();
                    p->setPen(QPen(indicatorBrush, 2));

                    p->drawLine(center.x(), center.y() - offset, center.x(), center.y() + offset);
                    p->drawLine(center.x() - offset, center.y(), center.x() + offset, center.y());
                }
                if (spinBox->subControls & SC_SpinBoxDown) {
                    center = downRect.center();
                    // offset for pressed state
                    if (downSunken) {
                        ++center.rx();
                        ++center.ry();
                    }

                    // Choose correct palette for - symbol
                    QPalette pal = spinBox->palette;
                    if (!(spinBox->stepEnabled & QAbstractSpinBox::StepDownEnabled)) {
                        pal.setCurrentColorGroup(QPalette::Disabled);
                    }
                    // Draw - symbol
                    QBrush indicatorBrush = pal.buttonText();
                    p->setPen(QPen(indicatorBrush, 2));

                    p->drawLine(center.x() - offset, center.y(), center.x() + offset, center.y());
                }
            }

            // Draw the separator lines between the edit rect and the buttons
            QPen separatorPen(Qt::DashLine);
            p->setPen( separatorPen );

            p->drawLine( frameEditRect.topRight() + QPoint(1,0),
                         frameEditRect.bottomRight() + QPoint(1,0) );
            p->drawLine( frameEditRect.topLeft() - QPoint(1,0),
                         frameEditRect.bottomLeft() - QPoint(1,0) );

            p->restore();
        }
    }
    break;
    case CC_ScrollBar: {
        drawControl(CE_ScrollBarSlider, opt, p, widget);
    }
    break;
    case CC_GroupBox: {
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(opt)) {
            const MContainerStyle *groupBoxStyle =
                static_cast<const MContainerStyle *>(QtMaemo6StylePrivate::mStyle(groupBox->state,
                        "MContainerStyle"));
            //causes problems because requires to include a private header file
            //MContainerHeader containerHeaderLabel;
            //const MLabelStyle * headerLabelStyle =
            //    static_cast<const MLabelStyle *>( QtMaemo6StylePrivate::mStyle( groupBox->state,
            //                                                    "MLabelStyle", "", "", &containerHeaderLabel ) );

            const MLabelStyle *headerStyle =
                static_cast<const MLabelStyle *>(QtMaemo6StylePrivate::mStyle(groupBox->state,
                                                   "MContainerHeaderStyle"));

            const MSeparatorStyle *separatorStyle =
                static_cast<const MSeparatorStyle *>(QtMaemo6StylePrivate::mStyle(groupBox->state,
                        "MSeparatorStyle"));

            if(groupBoxStyle && headerStyle && separatorStyle) {
                QRect headerTextRect = proxy()->subControlRect(CC_GroupBox, opt, SC_GroupBoxLabel, widget);
                QRect checkBoxRect = proxy()->subControlRect(CC_GroupBox, opt, SC_GroupBoxCheckBox, widget);

                int headMarginLeft =      headerStyle->paddingLeft()
                                          + headerStyle->marginLeft();
                int headMarginRight =     headerStyle->paddingRight()
                                          + headerStyle->marginRight();
                int headMarginBottom =    headerStyle->paddingBottom()
                                          + headerStyle->marginBottom()
                                          + 4; // Make it look good fix (while the header text padding in mcontainer is broken)

                QRect unitedRect = headerTextRect.united(checkBoxRect);
                int headerBottom = unitedRect.bottom() + headMarginBottom;

                int sepMarginLeft   =  separatorStyle->marginLeft() + separatorStyle->paddingLeft();
                int sepMarginRight  =  separatorStyle->marginRight() + separatorStyle->paddingRight();
                int sepMarginTop    =  separatorStyle->marginTop() + separatorStyle->paddingTop();

                QRect separatorRect(groupBox->rect.left() + sepMarginLeft,
                                    headerBottom + sepMarginTop,
                                    groupBox->rect.width()
                                    - headMarginLeft - headMarginRight
                                    - sepMarginLeft - sepMarginRight,
                                    separatorStyle->span());

                // Draw frame
                // Draw widget background
                QRect headerRect = widget->rect();
                headerRect.setBottom(headerBottom);
                QRect containerRect = widget->rect();
                containerRect.adjust(0, headerRect.height(), 0, 0);

                drawBackground(p, groupBox, headerRect, headerStyle, widget);
                drawBackground(p, groupBox, containerRect, groupBoxStyle, widget);
    //                drawWidgetBackground( p, groupBox, separatorRect, separatorStyle );

                // Draw title
                if ((groupBox->subControls & QStyle::SC_GroupBoxLabel) && !groupBox->text.isEmpty()) {

                    QColor headerColor = Qt::white; //FIXME: = headerLabelStyle->color();
                    if (headerColor.isValid()) {
                        p->setPen(headerColor);
                    }

                    int alignment = int(groupBox->textAlignment);
                    if (!proxy()->styleHint(QStyle::SH_UnderlineShortcut, opt, widget))
                        alignment |= Qt::TextHideMnemonic;

                    p->drawText(headerTextRect, Qt::AlignHCenter | alignment | Qt::TextShowMnemonic,
                                groupBox->text);
                }

                // Draw checkbox
                if (groupBox->subControls & SC_GroupBoxCheckBox) {
                    d->drawCheckBox(p, QString(), QIcon(), checkBoxRect, groupBox);
                }
            }
        }
    }
    break;
    default: {
        QPlastiqueStyle::drawComplexControl(control, opt, p, widget);
    }
    break;
    }
}

QRect QtMaemo6Style::subControlRect(ComplexControl control,
                                    const QStyleOptionComplex *option,
                                    SubControl subControl,
                                    const QWidget *widget /* = 0*/) const
{
    if( !isStyled( widget ) ) {
        return QPlastiqueStyle::subControlRect(control, option, subControl, widget);
    }

    // Handle rare case where this method is invoked before the polish is invoked
    Q_D(const QtMaemo6Style);
    if (!d->m_isMInitialized) {
        mDebug("PlainQt Style") << Q_FUNC_INFO << "entered before initialization of MComponentData";
        return QRect();
    }

    switch (control) {
    case CC_ToolButton: {
    }
    break;

    case CC_Slider: {
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            int maxSliderThickness = proxy()->pixelMetric(PM_SliderControlThickness, slider, widget);
            int maxSliderLength = proxy()->pixelMetric(PM_SliderLength, slider, widget);
            bool isHorizontal = (slider->orientation == Qt::Horizontal);

            switch (subControl) {
            case SC_SliderHandle: {
                int span = (isHorizontal)
                           ? slider->rect.width() - maxSliderLength
                           : slider->rect.height() - maxSliderLength;

                int handlePos = sliderPositionFromValue(slider->minimum,
                                                        slider->maximum,
                                                        slider->sliderPosition,
                                                        span,
                                                        slider->upsideDown);
                if (isHorizontal)
                    return QRect(slider->rect.x() + handlePos,
                                 slider->rect.y(),
                                 maxSliderLength,
                                 maxSliderThickness);
                else
                    return QRect(slider->rect.x(),
                                 slider->rect.y() + handlePos,
                                 maxSliderThickness,
                                 maxSliderLength);
            }
            break;
            case SC_SliderGroove: {
                if (isHorizontal)
                    return QRect(slider->rect.x(),
                                 slider->rect.y(),
                                 slider->rect.width(),
                                 maxSliderThickness);
                else
                    return QRect(slider->rect.x(),
                                 slider->rect.y(),
                                 maxSliderThickness,
                                 slider->rect.height());
            }
            break;
            default:
                break;
            }
        }
    }
    break;
    case CC_SpinBox: {
        QRect rect;
        if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {

            int widthPlusMinus = spinBox->rect.height();

            switch (subControl) {
            case SC_SpinBoxUp:
                if (spinBox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                rect = visualRect(spinBox->direction, spinBox->rect, rect);
                rect.setRect(spinBox->rect.right() - widthPlusMinus,
                             spinBox->rect.top(),
                             widthPlusMinus,
                             spinBox->rect.height());
                rect = visualRect(spinBox->direction, spinBox->rect, rect);
                break;
            case SC_SpinBoxDown:
                if (spinBox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                rect = visualRect(spinBox->direction, spinBox->rect, rect);
                rect.setRect(spinBox->rect.left(),
                             spinBox->rect.top(),
                             widthPlusMinus,
                             spinBox->rect.height());
                rect = visualRect(spinBox->direction, spinBox->rect, rect);
                break;
            case SC_SpinBoxEditField: {
                    int frameLeft = 0;
                    int frameRight = 0;
                    int frameTop = 0;
                    int frameBottom = 0;

                    const MTextEditStyle *style =
                        static_cast<const MTextEditStyle *>(QtMaemo6StylePrivate::mStyle(QStyle::State_Sunken,
                                "MTextEditStyle"));

                    if (style) {
                        // Check the recommended border size ...
                        if (style->backgroundImage()) {
                            style->backgroundImage()->borders(&frameLeft, &frameRight,
                                                            &frameTop, &frameBottom);
                        }
                    }

                    if (spinBox->buttonSymbols != QAbstractSpinBox::NoButtons) {
                        rect = spinBox->rect.adjusted(widthPlusMinus, 0, -widthPlusMinus, 0);
                        frameLeft = 0;
                        frameRight = 0;
                    } else {
                        rect = spinBox->rect;
                    }
                    rect.adjust( frameLeft / 2, frameTop / 2,
                                 -frameRight / 2, -frameBottom / 2);
                    rect = visualRect(spinBox->direction, spinBox->rect, rect);
                    break;
                }
            default:
                break;
            }
        }
        return rect;
    }
    break;
    case CC_ScrollBar: {
        if (subControl == SC_ScrollBarSlider)
            return d->scrollBarSliderRect(option, widget);
        break;
    }
    case CC_GroupBox: {
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            switch (subControl) {
            case SC_GroupBoxFrame:
                // Fall Through
            case SC_GroupBoxContents: {
                QRect contentsRect;

                const MLabelStyle *headerStyle =
                    static_cast<const MLabelStyle *>(QtMaemo6StylePrivate::mStyle(groupBox->state,
                                                       "MLabelStyle",
                                                       "MContainerTitle"));

                QFont headerFont = headerStyle->font();
                QFontMetrics fontMetrics(headerFont);
                int height = fontMetrics.height();

                const MContainerStyle *groupBoxStyle =
                    static_cast<const MContainerStyle *>(QtMaemo6StylePrivate::mStyle(groupBox->state,
                            "MContainerStyle"));
                const MSeparatorStyle *separatorStyle =
                    static_cast<const MSeparatorStyle *>(QtMaemo6StylePrivate::mStyle(groupBox->state,
                            "MSeparatorStyle"));

                int sepMarginTop    =  separatorStyle->marginTop() + separatorStyle->paddingTop();
                int sepMarginBottom =  separatorStyle->marginBottom() + separatorStyle->paddingBottom();

                int leftMargin =    groupBoxStyle->paddingLeft()
                                    + groupBoxStyle->marginLeft();

                int rightMargin =   groupBoxStyle->paddingRight()
                                    + groupBoxStyle->marginRight();

                int topMargin =     groupBoxStyle->paddingTop()
                                    + groupBoxStyle->marginTop()
                                    + headerStyle->paddingTop()
                                    + headerStyle->marginTop()
                                    + sepMarginTop
                                    + separatorStyle->span()
                                    + sepMarginBottom;

                int checkMarkHeight = 0;

                bool hasCheckBox = groupBox->subControls & QStyle::SC_GroupBoxCheckBox;

                if (hasCheckBox) {
                    QStyleOptionButton checkBox;
                    checkBox.QStyleOption::operator=(*option);
                    checkMarkHeight = proxy()->pixelMetric(PM_IndicatorHeight, &checkBox, widget);
                }

                topMargin += qMax(height, checkMarkHeight);

                contentsRect = groupBox->rect.adjusted(leftMargin,
                                                       topMargin,
                                                       -rightMargin,
                                                       0);

                return contentsRect;
                break;
            }
            case SC_GroupBoxCheckBox:
                // Fall Through
            case SC_GroupBoxLabel: {
                QRect labelRect;

                const MLabelStyle *headerStyle =
                    static_cast<const MLabelStyle *>(QtMaemo6StylePrivate::mStyle(groupBox->state,
                                                       "MLabelStyle",
                                                       "MContainerTitle"));
                QFont headerFont = headerStyle->font();
                QFontMetrics fontMetrics(headerFont);
                int height = fontMetrics.height();
                int textWidth = fontMetrics.size(Qt::TextShowMnemonic, groupBox->text).width() + 1;

                const MContainerStyle *groupBoxStyle =
                    static_cast<const MContainerStyle *>(QtMaemo6StylePrivate::mStyle(groupBox->state,
                            "MContainerStyle"));

                int leftMargin =    groupBoxStyle->paddingLeft()
                                    + groupBoxStyle->marginLeft()
                                    + headerStyle->paddingLeft()
                                    + headerStyle->marginLeft();

                int rightMargin =   groupBoxStyle->paddingRight()
                                    + groupBoxStyle->marginRight()
                                    + headerStyle->paddingRight()
                                    + headerStyle->marginRight();

                int topMargin =     groupBoxStyle->paddingTop()
                                    + groupBoxStyle->marginTop()
                                    + headerStyle->paddingTop()
                                    + headerStyle->marginTop();

                //unused
                //int bottomMargin =   groupBoxStyle->paddingBottom()
                //                  + groupBoxStyle->marginBottom()
                //                  + headerStyle->paddingBottom()
                //                  + headerStyle->marginBottom();

                labelRect = groupBox->rect.adjusted(leftMargin,
                                                    topMargin,
                                                    -rightMargin,
                                                    0);

                labelRect.setHeight(height);

                // We need a QStyleOptionButton based style option in order
                // to get correct geometries.
                QStyleOptionButton checkBox;
                checkBox.QStyleOption::operator=(*option);
                bool hasCheckBox = groupBox->subControls & QStyle::SC_GroupBoxCheckBox;
                int checkMarkWidth = 0;
                if (hasCheckBox) {
                    checkMarkWidth =   proxy()->pixelMetric(PM_IndicatorWidth, &checkBox, widget);
                    + proxy()->pixelMetric(PM_CheckBoxLabelSpacing, &checkBox, widget);
                }
                QRect totalRect = alignedRect(groupBox->direction,
                                              groupBox->textAlignment,
                                              QSize(textWidth + checkMarkWidth, height),
                                              labelRect);

                // Adjust totalRect if checkbox is set
                if (hasCheckBox) {
                    int checkMarkHeight = proxy()->pixelMetric(PM_IndicatorHeight, &checkBox, widget);

                    bool leftToRight = (groupBox->direction == Qt::LeftToRight);
                    int left = 0;
                    // Adjust for check box
                    if (subControl == SC_GroupBoxCheckBox) {
                        left = leftToRight ? totalRect.left() : (totalRect.right() - checkMarkWidth);
                        int top = totalRect.top();
                        totalRect.setRect(left, top, checkMarkWidth, checkMarkHeight);
                        // Adjust for label
                    } else {
                        left = leftToRight ? (totalRect.left() + checkMarkWidth) : totalRect.left();
                        int top = totalRect.top() + (checkMarkHeight - fontMetrics.height()) / 2;
                        totalRect.setRect(left, top,
                                          totalRect.width() - checkMarkWidth, totalRect.height());
                    }
                }
                return totalRect;

                break;
            }
            default:
                break;
            }
        }
        break;
    }
    default:
        break;
    }
    return QPlastiqueStyle::subControlRect(control, option, subControl, widget);
}

QRect QtMaemo6Style::subElementRect(SubElement element,
                                    const QStyleOption* option,
                                    const QWidget* widget) const
{
    if( !isStyled( widget ) ) {
        return QPlastiqueStyle::subElementRect(element, option, widget);
    }

    // Handle rare case where this method is invoked before the polish is invoked
    Q_D(const QtMaemo6Style);
    if (!d->m_isMInitialized) {
        mDebug("PlainQt Style") << Q_FUNC_INFO << "entered before initialization of MComponentData";
        return QRect();
    }

    QRect retRect = QPlastiqueStyle::subElementRect(element, option, widget);
    switch(element) {
        case SE_LineEditContents: {
            if(widget) {
                const MTextEditStyle *style =
                        static_cast<const MTextEditStyle *>(QtMaemo6StylePrivate::mStyle(QStyle::State_Sunken,
                                "MTextEditStyle"));
                QFontMetrics fm(widget->font());
                if(style) {
                    const QLineEdit *lineEdit  = qobject_cast<const QLineEdit *>(widget);
                    if (lineEdit && lineEdit->layoutDirection() == Qt::RightToLeft)
                        retRect.adjust(style->paddingRight(), 0, -style->paddingLeft(), 0);
                    else // layoutDirection is Qt::LeftToRight/default
                         retRect.adjust(style->paddingLeft(), 0, -style->paddingRight(), 0);
                }
                if(fm.height() > retRect.height())
                    retRect.setHeight(fm.height());
            }
        }
        break;
        default:
            retRect = QPlastiqueStyle::subElementRect(element, option, widget);
    };
    return retRect;
}


const MFeedback * QtMaemo6Style::feedback(const QString& feedbackName, const QWidget *widget)
{
    if ( widget ) {

        // First check for dynamic properties
        if ( widget->property("NoMFeedback").isValid() )
            return 0;

        // Then check for the class type

        QString meegoClassString;

	// classes that are lower in the inheritance chain need to
	// be placed first to allow for the string to be overwritten.
        if(qobject_cast<const QWidget*>(widget))
            meegoClassString = "MWidgetStyle";

	// We don't map the QAbstract* widget classes since inherited
	// classes are often enough semantically quite different.

        if(qobject_cast<const QCheckBox*>(widget))
            meegoClassString = "MCheckboxStyle";

        if(qobject_cast<const QComboBox*>(widget))
            meegoClassString = "MComboBoxStyle";

        if(qobject_cast<const QDialog*>(widget))
            meegoClassString = "MDialogStyle";

        if(qobject_cast<const QLabel*>(widget))
            meegoClassString = "MLabelStyle";

        if(qobject_cast<const QListWidget*>(widget))
            meegoClassString = "MListStyle";

        if(qobject_cast<const QMenu*>(widget))
            meegoClassString = "MApplicationMenuStyle";

        if(qobject_cast<const QMessageBox*>(widget))
            meegoClassString = "MMessageBoxStyle";

        if(qobject_cast<const QProgressBar*>(widget))
            meegoClassString = "MProgressIndicatorStyle";

        if(qobject_cast<const QPushButton*>(widget))
            meegoClassString = "MButtonStyle";

        //mtf doesn't support tab bars, so we use MButtonStyle
        if(qobject_cast<const QTabBar*>(widget))
            meegoClassString = "MButtonStyle";

        if(qobject_cast<const QScrollArea*>(widget))
            meegoClassString = "MPannableWidgetStyle";

        if(qobject_cast<const QSlider*>(widget))
            meegoClassString = "MSliderStyle";

        if(qobject_cast<const QTextEdit*>(widget))
            meegoClassString = "MTextEditStyle";

        if(qobject_cast<const QToolBar*>(widget))
            meegoClassString = "MToolbarStyle";

	if ( meegoClassString.isEmpty() )
            return 0;

        QStyleOption opt;
        opt.initFrom(widget);

        const MWidgetStyle *style = static_cast<const MWidgetStyle *>(
            QtMaemo6StylePrivate::mStyle(opt.state, meegoClassString, ""));

        if ( feedbackName == MFeedback::Press )
            return &(style->pressFeedback());
        if ( feedbackName == MFeedback::Release )
            return &(style->releaseFeedback());
        if ( feedbackName == MFeedback::Cancel )
            return &(style->cancelFeedback());
    }

    return 0;
}

QSize QtMaemo6Style::sizeFromContents(ContentsType type,
                                      const QStyleOption *option,
                                      const QSize &contentsSize,
                                      const QWidget *widget) const
{
    if( !isStyled( widget ) ) {
        return QPlastiqueStyle::sizeFromContents(type, option, contentsSize, widget);
    }

    // Handle rare case where this method is invoked before the polish is invoked
    Q_D(const QtMaemo6Style);
    if (!d->m_isMInitialized) {
        mDebug("PlainQt Style") << Q_FUNC_INFO << "entered before initialization of MComponentData";
        return QSize();
    }

    QSize retSize = QPlastiqueStyle::sizeFromContents(type, option, contentsSize, widget);

    switch (type) {
    case CT_ScrollBar:
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {

            int scrollBarExtent = proxy()->pixelMetric(PM_ScrollBarExtent, option, widget);
            int scrollBarSliderMinimum = proxy()->pixelMetric(PM_ScrollBarSliderMin, option, widget);
            if (scrollBar->orientation == Qt::Horizontal) {
                retSize = QSize(scrollBarSliderMinimum, scrollBarExtent);
            } else {
                retSize = QSize(scrollBarExtent, scrollBarSliderMinimum);
            }
        }
        break;
    case CT_Slider: {
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            int sliderThickness = proxy()->pixelMetric(PM_SliderThickness, slider, widget);

            bool isHorizontal = (slider->orientation == Qt::Horizontal);
            if (isHorizontal)
                retSize.setHeight(sliderThickness);
            else
                retSize.setWidth(sliderThickness);
            return retSize;
        }
    }
    break;
    case CT_ComboBox:
    case CT_ToolButton:
        if (widget) {
            QWidget *parentOfControl = widget->parentWidget();

            if (const QStyleOptionToolButton *btn = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
                if(dynamic_cast<QtMaemo6TitleBar *>(parentOfControl))  {
                    //intentionally no code here at the moment
                    //a ToolButton anywhere else on the Screen should be drawn like a PushButton
                } else {
                    QStyleOptionToolButton subopt = *btn;
                    const MButtonStyle *style =
                        static_cast<const MButtonStyle *>(QtMaemo6StylePrivate::mStyle(subopt.state, "MButtonStyle"));
                    const MButtonStyle *styleFont =
                        static_cast<const MButtonStyle *>(QtMaemo6StylePrivate::mStyle(subopt.state, "MButtonStyle",
                                                            "NavigationBarToolBarButton"));

                    if(style && styleFont) {
                        QRect textAndIconRect = d->getTextAndIconRect(style, subopt.text, subopt.icon, styleFont->font(), styleFont->iconSize());
                        textAndIconRect.setWidth(textAndIconRect.width() + style->textMarginLeft() + style->textMarginRight());
                        textAndIconRect.setHeight(style->preferredSize().height());
                        retSize = textAndIconRect.size();
                    } else {
                        qCritical() << "MButtonStyle for QToolButton could not be loaded";
                    }
                }
            }
        }
        break;
    case CT_PushButton: {
        if (option) {
            QStyleOption subopt = *option;
            const MWidgetStyle *style;
            if(qobject_cast<const QPushButton *>(widget))
                style = static_cast<const MWidgetStyle *>(QtMaemo6StylePrivate::mStyle(subopt.state, "MButtonStyle"));
             else
                style = static_cast<const MWidgetStyle *>(QtMaemo6StylePrivate::mStyle(subopt.state, "MComboBoxStyle"));

            if(style) {
                QSize bs = d->heuristicSizeFromStyle(style);
                if (bs.isValid()) {
                    bs.setWidth(bs.width() + style->marginLeft() + style->marginRight());
                      bs.setHeight(bs.height() + style->marginBottom() + style->marginTop());
                } else {
                    bs = QSize(500, 100);
                }
                retSize = bs;
            } else {
                qCritical() << "MButtonStyle for QPushButton could not be loaded";
            }
        }
    }
    break;
    case CT_CheckBox: {
        //ToDo
    }
    break;
    case CT_ItemViewItem:
        if (const QStyleOptionViewItemV4 *item = qstyleoption_cast<const QStyleOptionViewItemV4 *>(option)) {
            const MContentItemStyle *style =
                static_cast<const MContentItemStyle *>(QtMaemo6StylePrivate::mStyle(option->state,
                        "MContentItemStyle"));
            const MLabelStyle *labelTitle =
                static_cast<const MLabelStyle *>(QtMaemo6StylePrivate::mStyle(option->state,
                                                   "MLabelStyle", style->titleObjectName()));
            // subtitles for ListViewItems are currently not provided by qt.
            //const MLabelStyle* labelSubTitle =
            //        static_cast<const MLabelStyle*>( QtMaemo6StylePrivate::mStyle( option->state,
            //                                               "MLabelStyle", style->subtitleObjectName() ) );
            const MImageWidgetStyle *labelIcon =
                static_cast<const MImageWidgetStyle *>(QtMaemo6StylePrivate::mStyle(option->state,
                        "MImageWidgetStyle", style->imageObjectName()));

            QString itemText = qvariant_cast<QString>(item->index.data(Qt::DisplayRole)); //.toString();
            QIcon itemIcon = qvariant_cast<QIcon>(item->index.data(Qt::DecorationRole));

            //always calculate icon size, even if the index has no item,
            // to get equal row heights
            QSize iconSize = labelIcon->preferredSize() +
                             QSize(labelIcon->marginLeft() + labelIcon->marginRight(),
                                   labelIcon->marginTop() + labelIcon->marginBottom());

            QFontMetrics fontMetrics(labelTitle->font());
            QRect textRect = fontMetrics.boundingRect(itemText);
            textRect.setWidth(textRect.width() +
                              labelTitle->marginLeft() + labelTitle->marginRight());

            retSize.setWidth(textRect.width() + iconSize.width());
            if (iconSize.height() < textRect.height())
                retSize.setHeight(textRect.height());
            else
                retSize.setHeight(iconSize.height());

        }
        break;
    case CT_MenuBar: {
        //return 0 size because the menubar should not be drawn
            return QSize(0, 0);
        // this is done by the windowdecoration
        /*
        if (widget == d->m_menuBar) {
            const MNavigationBarStyle *style =
                static_cast<const MNavigationBarStyle *>(QtMaemo6StylePrivate::mStyle(QStyle::State_Active,
                        "MNavigationBarStyle"));
            int navBarHeight = 90;
            if(style)
                navBarHeight = style->minimumSize().height();
            navBarHeight += 30; //height of statusbar
            retSize = QSize(navBarHeight, navBarHeight);
        }
        */
    }
    break;
    case CT_LineEdit: {
        retSize = QPlastiqueStyle::sizeFromContents(type, option, contentsSize, widget);
        if(widget) {
            QFontMetrics fm(widget->font());

            const MTextEditStyle *style =
                  static_cast<const MTextEditStyle *>(QtMaemo6StylePrivate::mStyle(QStyle::State_Sunken,
                          "MTextEditStyle"));
            int requiredHeight = fm.height();
            if(style) {
                requiredHeight += style->paddingTop() + style->paddingBottom();
            }
            if(retSize.height() < requiredHeight)
                retSize.setHeight(requiredHeight);
        }
    }
    break;
    default:
        break;
    }
    return retSize;
}

int QtMaemo6Style::pixelMetric(PixelMetric metric,
                               const QStyleOption *option,
                               const QWidget *widget) const
{
    if ( !isStyled( widget ) ) {
        return QPlastiqueStyle::pixelMetric(metric, option, widget);
    }

    // Handle rare case where this method is invoked before the polish is invoked
    Q_D(const QtMaemo6Style);
    if (!d->m_isMInitialized) {
        mDebug("PlainQt Style") << Q_FUNC_INFO << "entered before initialization of MComponentData";
        return  QPlastiqueStyle::pixelMetric(metric, option, widget);
    }

    switch (metric) {
    case PM_ScrollBarExtent:
    case PM_ScrollBarSliderMin: {
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const MPositionIndicatorStyle *style =
                static_cast<const MPositionIndicatorStyle *>(QtMaemo6StylePrivate::mStyle(slider->state,
                        "MPositionIndicatorStyle"));

            if(style && style->indicatorImage() && style->indicatorImage()->pixmap()) {
                int minSize = 0;
                if (slider->orientation == Qt::Vertical)
                    minSize = style->preferredSize().width();
                else
                    minSize = style->preferredSize().height();

                return minSize;
            }
        }
    }
    break;
    case PM_SliderThickness:
    case PM_SliderControlThickness: {
        int sliderThickness = 0;

        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const MSliderStyle *style =
                static_cast<const MSliderStyle *>(QtMaemo6StylePrivate::mStyle(slider->state,
                                                    "MSliderStyle"));
            if (style) {
                bool isHorizontal = slider->orientation == Qt::Horizontal;
                const QPixmap *handleSourcePixmap = (isHorizontal)
                                                    ? style->handlePixmap()
                                                    : style->handleVerticalPixmap();
                const QPixmap *handlePressedSourcePixmap = (isHorizontal)
                        ? style->handlePressedPixmap()
                        : style->handleVerticalPressedPixmap();

                QSize sliderHandleSize = handleSourcePixmap->size();
                QSize sliderHandlePressedSize = handlePressedSourcePixmap->size();
                QSize maxHandleSize = sliderHandleSize.expandedTo(sliderHandlePressedSize);
                sliderThickness = maxHandleSize.height();
            }
        }
        return sliderThickness;
    }
    break;
    case PM_SliderLength: {
        int sliderLength = 0;

        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const MSliderStyle *style =
                static_cast<const MSliderStyle *>(QtMaemo6StylePrivate::mStyle(slider->state,
                                                    "MSliderStyle"));
            if (style) {
                bool isHorizontal = slider->orientation == Qt::Horizontal;
                const QPixmap *handleSourcePixmap = (isHorizontal)
                                                    ? style->handlePixmap()
                                                    : style->handleVerticalPixmap();
                const QPixmap *handlePressedSourcePixmap = (isHorizontal)
                        ? style->handlePressedPixmap()
                        : style->handleVerticalPressedPixmap();

                QSize sliderHandleSize = handleSourcePixmap->size();
                QSize sliderHandlePressedSize = handlePressedSourcePixmap->size();
                QSize maxHandleSize = sliderHandleSize.expandedTo(sliderHandlePressedSize);
                sliderLength = maxHandleSize.width();
            }
        }
        return sliderLength;
    }
    break;
    case PM_ExclusiveIndicatorWidth:
    case PM_IndicatorWidth: {

        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            const MButtonStyle *style =
                static_cast<const MButtonStyle *>(QtMaemo6StylePrivate::mStyle(btn->state,
                                                    "MCheckboxStyle",
                                                    QString(),
                                                    "checkbox"));

            if (style) {
                return d->heuristicSizeFromStyle(style).width();
            }
        }
    }
    break;
    case PM_ExclusiveIndicatorHeight:
    case PM_IndicatorHeight: {

        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            const MButtonStyle *style =
                static_cast<const MButtonStyle *>(QtMaemo6StylePrivate::mStyle(btn->state,
                                                    "MCheckboxStyle",
                                                    QString(),
                                                    "checkbox"));
            if (style) {
                return d->heuristicSizeFromStyle(style).height();
            }
        }
    }
    break;
    case PM_TabBarScrollButtonWidth:
        return widget->height();
    default:
        break;
    }

    return QPlastiqueStyle::pixelMetric(metric, option, widget);
}

QIcon QtMaemo6Style::standardIconImplementation(StandardPixmap standardIcon, const QStyleOption *option,
        const QWidget *widget) const
{
    if ( !isStyled( widget ) ) {
        return QPlastiqueStyle::standardIconImplementation(standardIcon, option, widget);
    }

    QIcon   icon;
    QPixmap pixmap;

    switch (standardIcon) {
    case QStyle::SP_MessageBoxInformation: // 09
        return QIcon();
    case QStyle::SP_MessageBoxWarning:   // 10
        return QIcon();
    case QStyle::SP_MessageBoxCritical:  // 11
        return QIcon();
    case QStyle::SP_MessageBoxQuestion:  // 12
        return QIcon();
    case QStyle::SP_DialogOkButton:      // 38
        return QIcon();
    case QStyle::SP_DialogCancelButton:  // 39
        return QIcon();
    case QStyle::SP_DialogHelpButton:    // 40
        return QIcon();
    case QStyle::SP_DialogOpenButton:    // 41
        return QIcon();
    case QStyle::SP_DialogSaveButton:    // 42
        return QIcon();
    case QStyle::SP_DialogCloseButton:   // 43
        return QIcon();
    case QStyle::SP_DialogApplyButton:   // 44
        return QIcon();
    case QStyle::SP_DialogResetButton:   // 45
        return QIcon();
    case QStyle::SP_DialogDiscardButton: // 46
        return QIcon();
    case QStyle::SP_DialogYesButton:     // 47
        return QIcon();
    case QStyle::SP_DialogNoButton:      // 48
        return QIcon();
    default:
        break;
    }

    if (icon.isNull())
        icon = QCommonStyle::standardIconImplementation(standardIcon, option, widget);
    return icon;
}

int QtMaemo6Style::styleHint(StyleHint hint, const QStyleOption *option,
                             const QWidget *widget, QStyleHintReturn *returnData) const
{
    if( !isStyled( widget ) ) {
        return QPlastiqueStyle::styleHint(hint, option, widget, returnData);
    }

    if (hint == QStyle::SH_ToolBar_Movable) {
        return false;
    } else if (hint == QStyle::SH_RequestSoftwareInputPanel) {
        return QStyle::RSIP_OnMouseClick;
    }

    return QPlastiqueStyle::styleHint(hint, option, widget, returnData);
}

void QtMaemo6Style::ensureFocusedWidgetVisible(QRect rect)
{
    Q_D(QtMaemo6Style);

    // don't scroll if hardware keyboard is open
    if (MKeyboardStateTracker::instance()->isOpen()) {
        if (!d->m_originalWidgetPos.widget.isNull()) {
            d->m_originalWidgetPos.widget->move(d->m_originalWidgetPos.position);
            d->m_originalWidgetPos.widget = 0;
        }        
    } else if (rect.size().isValid() && !d->m_originalWidgetPos.widget.isNull()) {
        // restore original widget position
        d->m_originalWidgetPos.widget->move(d->m_originalWidgetPos.position);
        d->m_originalWidgetPos.widget = 0;
    } else {
        QWidget* widget = qApp->focusWidget();
        if(widget) {
            if(rect.isValid()) {
                QRect screenRect = QRect(QPoint(0,0), MDeviceProfile::instance()->resolution());
                int spaceAbove = rect.top();
                int spaceBelow = screenRect.height() - (rect.top() + rect.height());

                if(spaceAbove > spaceBelow) {
                    int maxY = rect.top();
                    d->ensureWidgetVisible(widget, QRect(QPoint(0,0), QSize(screenRect.width(), maxY)));
                } else {
                    int minY = rect.top() + rect.height();
                    d->ensureWidgetVisible(widget, QRect(QPoint(0,minY), QSize(screenRect.width(), screenRect.height()-minY)));
                }
            } 
        }
    }
}

void QtMaemo6StylePrivate::ensureWidgetVisible(QWidget* widget, QRect visibleRect)
{
    QWidget* topLevelWidget = widget->window();
    if(topLevelWidget) {

        QRect globalVisibleRect = visibleRect; 

        QPoint widgetGlobalPosition = widget->mapToGlobal(QPoint(0,0));
        QRect globalWidgetRect = QRect(widgetGlobalPosition, widget->size());

        //the widget is not fully covered by the visible Area
        if(!globalVisibleRect.contains(globalWidgetRect, true)) {

            if (!m_originalWidgetPos.widget) {
                m_originalWidgetPos.widget = topLevelWidget;
                m_originalWidgetPos.position = topLevelWidget->pos();
            }

            int newYPos = globalVisibleRect.top() + (globalVisibleRect.height() / 2 - widget->height() / 2);
            QPoint moveBy = QPoint(topLevelWidget->geometry().x(), topLevelWidget->geometry().y()-(widgetGlobalPosition.y() - newYPos));

            //window centered in visibleArea
            topLevelWidget->move(moveBy);
        } 
    } else {
        mDebug("PlainQt Style") << "Can't focus on" << widget << "because there is no QMainWindow";
    }
}

void QtMaemo6Style::doOrientationChange()
{
    Q_D(QtMaemo6Style);
    if(d->m_isOrientationChangeEnabled) {
        M::OrientationAngle currentOrientation = orientation();
        MInputMethodState::instance()->setActiveWindowOrientationAngle(currentOrientation);
        emit orientationChanged(currentOrientation);
        //also emit this with int as parameter for meegotouch independent use
        emit orientationChanged(static_cast<int>(currentOrientation));
    }
}

void QtMaemo6Style::updateDirtyWidgets() {
    Q_D(QtMaemo6Style);
    if(!d->m_dirtyWidgets.isEmpty()) {
        foreach(QWidget* w, d->m_dirtyWidgets.keys()) { // krazy:exclude=foreach
            const MScalableImage* image = d->m_dirtyWidgets.take(w);
            //if there is an image in the cache list, set it as background for the widget
            if(image)
                setPaletteBackground(w, image);
            w->update();
        }
    }
}

/*
    Private implementation specific methods:
 */








