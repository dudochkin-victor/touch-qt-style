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

#ifndef QTMAEMO6STYLE_H
#define QTMAEMO6STYLE_H

#include <QFont>
#include <QMap>
#include <QLabel>
#include <QPlastiqueStyle>

#include <qtmaemo6export.h>
#include <mnamespace.h>

class MComponentData;
class MStyle;
class MButtonStyle;
class MFeedback;
class MWidgetStyle;
class MScalableImage;
class MSliderStyle;
class QScrollBar;
class QTimer;
class QSequentialAnimationGroup;
class QAbstractScrollArea;

class QtMaemo6StylePrivate;


/*!
 * This global function adds opacity to a pixmap
 */
QPixmap setPixmapOpacity(const QPixmap &pixmap, double opacity);

/*!
 * This class emulates the M windowdecoration, it provides a emulation
 * of the M titlebar, adds scrolling functionality and places a statusbar
 * on the bottom of the screen, if the application has one.
 * This class inherits indirectly from QPlastiqueStyle and reimplements
 * necessarry methods for doing the correct m-like styling.
 * Currently it inherits from QtMaemo6TestStyle (inherits QPlastiqueStyle),
 * which is currently only for testing purposes and may be deprecated in future.
 */
class M_QT_STYLE_EXPORT QtMaemo6Style : public QPlastiqueStyle
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QtMaemo6Style)
    friend class QtMaemo6StyleEventFilter;
    Q_PROPERTY(bool orientationChangeEnabled READ isOrientationChangeEnabled WRITE setOrientationChangeEnabled);
public:
    QtMaemo6Style();
    virtual ~QtMaemo6Style();

    /*! \reimp */
    virtual void init();

    virtual void polish(QApplication *app);
    virtual void polish(QWidget *w);

    virtual void drawPrimitive(PrimitiveElement element,
                               const QStyleOption *option,
                               QPainter *painter,
                               const QWidget *widget = 0) const;

    virtual void drawControl(ControlElement element,
                             const QStyleOption *option,
                             QPainter *painter,
                             const QWidget *widget = 0) const;

    virtual void drawComplexControl(ComplexControl control,
                                    const QStyleOptionComplex *option,
                                    QPainter *painter,
                                    const QWidget *widget = 0) const;

    QSize sizeFromContents(ContentsType type,
                           const QStyleOption *option,
                           const QSize &contentsSize,
                           const QWidget *widget = 0) const;

    int pixelMetric(PixelMetric metric,
                    const QStyleOption *option = 0,
                    const QWidget *widget = 0) const;

    virtual QRect subControlRect(ComplexControl control,
                                 const QStyleOptionComplex *option,
                                 SubControl subControl,
                                 const QWidget *widget = 0) const;

    virtual QRect subElementRect(SubElement element,
                                 const QStyleOption* option,
                                 const QWidget* widget = 0 ) const;

    const MFeedback * feedback(const QString &feedbackName, const QWidget *widget = 0);

    /*! \reimp_end */

public Q_SLOTS:
    bool isStyled( const QWidget * widget = 0 ) const;

    /*!
     * \brief: returns true if the orientation change support is enabled
     */
    bool isOrientationChangeEnabled() const;

    /*!
     * \brief: set to true to enable orientation change support
     */
    void setOrientationChangeEnabled(bool b);

    /*!
     * \brief Returns the current orientation of the device
     */
    M::OrientationAngle orientation();

    /*!
     * \brief returns the current orientation of the device within the given int
     *        for external meegotouch independent use
     * this gives you the orientation angle as int, so the value might be 0, 90,
     * 180 or 270.
     */
    void orientation(int*);

    /*!
     * sets the background image from the style to the palette of the widget
     * returns true if the image is loaded and drawn
     * returns false if the image is not loaded yet (and then of course wasn't drawn)
     *               but the loading process was invoked then.
     */
    bool setPaletteBackground(QWidget *,
                              QString styleObject = QString(),
                              QString styleClass = QString()) const;
    bool setPaletteBackground(QWidget *,
                              const MScalableImage*) const;
    bool drawBackground      (QPainter *p,
                              const QStyleOption *option,
                              const QRect &rect,
                              const MWidgetStyle *style,
                              const QWidget* w = 0) const;

Q_SIGNALS:
    void orientationChanged(M::OrientationAngle angle);
    void orientationChanged(int);

protected Q_SLOTS:
    /*! \reimp */
    QIcon standardIconImplementation(StandardPixmap standardIcon, const QStyleOption *option,
                                     const QWidget *widget = 0) const;

    virtual int styleHint(StyleHint hint, const QStyleOption *option = 0,
                          const QWidget *widget = 0, QStyleHintReturn *returnData = 0) const;
    /*! \reimp_end */

    void ensureFocusedWidgetVisible(QRect rect);

    void doOrientationChange();

    /*!
     * \brief update all widgets marked as dirty
     *
     * How asynchronous image loading and painting works within the QtMaemo6 style:
     * When calling QtMaemo6StylePrivate::drawScalableImage() the image may not be
     * painted imediately, because it is not loaded yet. In this case, the widget
     * will be remembered as dirty.
     * If you provide an explicit widget to drawScalableImage(), it is assumed you
     * want to use the image as background of the widget, which means just repainting
     * the widget isn't enough but the palette of the widget has to be changed.
     * This is done by the style  automaticall if you provide the widget here.
     * If no widget is provided, the widget just receives an update when the image is
     * loaded, but it is not set to the palette's background role.
     *
     * \see QtMaemo6StylePrivate::drawScalableImage()
     */
    void updateDirtyWidgets();

protected:
    QtMaemo6StylePrivate *const d_ptr;
    QtMaemo6Style(QtMaemo6StylePrivate &dd);
};

#endif
