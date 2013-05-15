;/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QPointer>
#include <QObject>
#include <QtCore/qmath.h>

#include "qtscrollerproperties.h"
#include "qtscrollerproperties_p.h"

static QtScrollerPropertiesPrivate *userDefaults = 0;
static QtScrollerPropertiesPrivate *systemDefaults = 0;

QtScrollerPropertiesPrivate *QtScrollerPropertiesPrivate::defaults()
{
    if (!systemDefaults) {
        QtScrollerPropertiesPrivate spp;
#ifdef Q_WS_MAEMO_5
        spp.dragVelocitySmoothingFactor = qreal(0.15);
        spp.overshootDragResistanceFactor = qreal(1);
        spp.overshootMaximumDistanceFactor = qreal(0.3);
        spp.snapPositionRatio = qreal(0.25);
        spp.snapTime = qreal(1);
        spp.mousePressEventDelay = qreal(0);
        spp.dragStartDistance = qreal(2.5 / 1000);
        spp.minimumVelocity = qreal(0.0195);
        spp.maximumVelocity = qreal(6.84);
        spp.maximumClickThroughVelocity = qreal(0.0684);
        spp.axisLockThreshold = qreal(0);
        spp.acceleratingFlickMaximumTime = qreal(0.125);
        spp.acceleratingFlickSpeedupFactor = qreal(3.0);
        spp.hOvershootPolicy = QScrollerProperties::OvershootWhenScrollable;
        spp.vOvershootPolicy = QScrollerProperties::OvershootWhenScrollable;

        spp.scrollingCurve.setType(QEasingCurve::OutQuad);
        spp.decelerationFactor = 1.0;

        spp.overshootTime = qreal(0.5);
        spp.overshootCurve.setType(QEasingCurve::OutQuad);

#else
        spp.dragVelocitySmoothingFactor = qreal(0.02);
        spp.overshootDragResistanceFactor = qreal(0.5);
        spp.overshootMaximumDistanceFactor = qreal(0.5);
        spp.snapPositionRatio = qreal(0.5);
        spp.snapTime = qreal(1);
        spp.mousePressEventDelay = qreal(0.25);
        spp.dragStartDistance = qreal(5.0 / 1000);
        spp.minimumVelocity = qreal(50.0 / 1000);
        spp.maximumVelocity = qreal(500.0 / 1000);
        spp.maximumClickThroughVelocity = qreal(66.5 / 1000);
        spp.axisLockThreshold = qreal(0);
        spp.acceleratingFlickMaximumTime = qreal(1.25);
        spp.acceleratingFlickSpeedupFactor = qreal(3.0);
        spp.hOvershootPolicy = QtScrollerProperties::OvershootWhenScrollable;
        spp.vOvershootPolicy = QtScrollerProperties::OvershootWhenScrollable;

        spp.scrollingCurve.setType(QEasingCurve::OutQuad);
        spp.decelerationFactor = qreal(0.125);

        spp.overshootTime = qreal(0.7);
        spp.overshootCurve.setType(QEasingCurve::OutQuad);
#endif
        systemDefaults = new QtScrollerPropertiesPrivate(spp);
    }
    return new QtScrollerPropertiesPrivate(userDefaults ? *userDefaults : *systemDefaults);
}

/*!
    \class QScrollerProperties
    \brief The QScrollerProperties class stores the settings for a QScroller.
    \since 4.8
    \preliminary

    The QScrollerProperties class stores the parameters used by QScroller.

    The default settings are platform dependant and Qt will emulate the
    platform behaviour for kinetic scrolling.

    As a convention the QScrollerProperties are in physical units (meter,
    seconds) and will be converted by QScroller using the current DPI.

    \sa QScroller
*/

/*!
    Constructs new scroller properties.
*/
QtScrollerProperties::QtScrollerProperties()
    : d(QtScrollerPropertiesPrivate::defaults())
{
}

/*!
    Copy constructor for the scroller properties.
*/
QtScrollerProperties::QtScrollerProperties(const QtScrollerProperties &sp)
    : d(new QtScrollerPropertiesPrivate(*sp.d))
{
}

QtScrollerProperties &QtScrollerProperties::operator=(const QtScrollerProperties &sp)
{
    *d.data() = *sp.d.data();
    return *this;
}

/*!
    Destroys the scroller properties.
*/
QtScrollerProperties::~QtScrollerProperties()
{
}

bool QtScrollerProperties::operator==(const QtScrollerProperties &sp) const
{
    return *d.data() == *sp.d.data();
}

bool QtScrollerProperties::operator!=(const QtScrollerProperties &sp) const
{
    return !(*d.data() == *sp.d.data());
}

bool QtScrollerPropertiesPrivate::operator==(const QtScrollerPropertiesPrivate &p) const
{
    bool same = true;
    same &= (dragVelocitySmoothingFactor == p.dragVelocitySmoothingFactor);
    same &= (overshootMaximumDistanceFactor == p.overshootMaximumDistanceFactor);
    same &= (overshootDragResistanceFactor == p.overshootDragResistanceFactor);
    same &= (snapPositionRatio == p.snapPositionRatio);
    same &= (snapTime == p.snapTime);
    same &= (mousePressEventDelay == p.mousePressEventDelay);
    same &= (dragStartDistance == p.dragStartDistance);
    same &= (minimumVelocity == p.minimumVelocity);
    same &= (maximumVelocity == p.maximumVelocity);
    same &= (maximumClickThroughVelocity == p.maximumClickThroughVelocity);
    same &= (axisLockThreshold == p.axisLockThreshold);
    same &= (acceleratingFlickMaximumTime == p.acceleratingFlickMaximumTime);
    same &= (acceleratingFlickSpeedupFactor == p.acceleratingFlickSpeedupFactor);
    same &= (scrollingCurve == p.scrollingCurve);
    same &= (decelerationFactor == p.decelerationFactor);
    same &= (overshootTime == p.overshootTime);
    same &= (overshootCurve == p.overshootCurve);
    same &= (hOvershootPolicy == p.hOvershootPolicy);
    same &= (vOvershootPolicy == p.vOvershootPolicy);
    return same;
}

/*!
     Sets the scroller properties returned by the default constructor to \a sp.

     Use this function to override the platform default properties returned by the default
     constructor. If you only want to change the scroller properties of a single scroller, then use
     QScroller::setScrollerProperties()

     \note Calling this function will not change the content of already existing
     QScrollerProperties objects.

     \sa unsetDefaultScrollerProperties()
*/
void QtScrollerProperties::setDefaultScrollerProperties(const QtScrollerProperties &sp)
{
    if (!userDefaults)
        userDefaults = new QtScrollerPropertiesPrivate(*sp.d);
    else
        *userDefaults = *sp.d;
}

/*!
     Sets the scroller properties returned by the default constructor back to the platform default
     properties.

     \sa setDefaultScrollerProperties
*/
void QtScrollerProperties::unsetDefaultScrollerProperties()
{
    delete userDefaults;
    userDefaults = 0;
}

/*!
    Query the \a metric value of the scroller properties.

    \sa setScrollMetric(), ScrollMetric
*/
QVariant QtScrollerProperties::scrollMetric(ScrollMetric metric) const
{
    switch (metric) {
    case DragVelocitySmoothingFactor:   return d->dragVelocitySmoothingFactor;
    case OvershootDragResistanceFactor: return d->overshootDragResistanceFactor;
    case OvershootMaximumDistanceFactor:return d->overshootMaximumDistanceFactor;
    case SnapPositionRatio:             return d->snapPositionRatio;
    case SnapTime:                      return d->snapTime;
    case MousePressEventDelay:          return d->mousePressEventDelay;
    case DragStartDistance:             return d->dragStartDistance;
    case MinimumVelocity:               return d->minimumVelocity;
    case MaximumVelocity:               return d->maximumVelocity;
    case MaximumClickThroughVelocity:   return d->maximumClickThroughVelocity;
    case AxisLockThreshold:             return d->axisLockThreshold;
    case AcceleratingFlickMaximumTime:  return d->acceleratingFlickMaximumTime;
    case AcceleratingFlickSpeedupFactor:return d->acceleratingFlickSpeedupFactor;
    case ScrollingCurve:                return QVariant::fromValue(d->scrollingCurve);
    case DecelerationFactor:            return d->decelerationFactor;
    case OvershootTime:                 return d->overshootTime;
    case OvershootCurve:                return QVariant::fromValue(d->overshootCurve);
    case ScrollMetricCount:             break;
    }
    return QVariant();
}

/*!
    Set a specific value of the \a metric ScrollerMetric to \a value.

    \sa scrollMetric(), ScrollMetric
*/
void QtScrollerProperties::setScrollMetric(ScrollMetric metric, const QVariant &value)
{
    switch (metric) {
    case DragVelocitySmoothingFactor:   d->dragVelocitySmoothingFactor = qBound(qreal(0), value.toReal(), qreal(1)); break;
    case OvershootDragResistanceFactor: d->overshootDragResistanceFactor = value.toReal(); break;
    case OvershootMaximumDistanceFactor:d->overshootMaximumDistanceFactor = qBound(qreal(0), value.toReal(), qreal(1)); break;
    case SnapPositionRatio:             d->snapPositionRatio = qBound(qreal(0), value.toReal(), qreal(1)); break;
    case SnapTime:                      d->snapTime = value.toReal(); break;
    case MousePressEventDelay:          d->mousePressEventDelay = value.toReal(); break;
    case DragStartDistance:             d->dragStartDistance = value.toReal(); break;
    case MinimumVelocity:               d->minimumVelocity = value.toReal(); break;
    case MaximumVelocity:               d->maximumVelocity = value.toReal(); break;
    case MaximumClickThroughVelocity:   d->maximumClickThroughVelocity = value.toReal(); break;
    case AxisLockThreshold:             d->axisLockThreshold = qBound(qreal(0), value.toReal(), qreal(1)); break;
    case AcceleratingFlickMaximumTime:  d->acceleratingFlickMaximumTime = value.toReal(); break;
    case AcceleratingFlickSpeedupFactor:d->acceleratingFlickSpeedupFactor = value.toReal(); break;
    case ScrollingCurve:                d->scrollingCurve = value.value<QEasingCurve>(); break;
    case DecelerationFactor:            d->decelerationFactor = value.toReal(); break;
    case OvershootTime:                 d->overshootTime = value.toReal(); break;
    case OvershootCurve:                d->overshootCurve = value.value<QEasingCurve>(); break;
    case ScrollMetricCount:             break;
    }
}

/*!
    Returns the horizontal overshooting policy.

    \sa setHorizontalOvershootPolicy()
*/
QtScrollerProperties::OvershootPolicy QtScrollerProperties::horizontalOvershootPolicy() const
{
    return d->hOvershootPolicy;
}

/*!
    Set the horizontal overshooting policy to \a policy.

    \sa horizontalOvershootPolicy()
*/
void QtScrollerProperties::setHorizontalOvershootPolicy(QtScrollerProperties::OvershootPolicy policy)
{
    d->hOvershootPolicy = policy;
}

/*!
    Returns the vertical overshooting policy.

    \sa setVerticalOvershootPolicy()
*/
QtScrollerProperties::OvershootPolicy QtScrollerProperties::verticalOvershootPolicy() const
{
    return d->vOvershootPolicy;
}

/*!
    Set the vertical overshooting policy to \a policy.

    \sa verticalOvershootPolicy()
*/
void QtScrollerProperties::setVerticalOvershootPolicy(QtScrollerProperties::OvershootPolicy policy)
{
    d->vOvershootPolicy = policy;
}

/*!
    \enum QScrollerProperties::OvershootPolicy

    This enum describes the various modes of overshooting.

    \value OvershootWhenScrollable Overshooting is when the content is
                                   scrollable.  This is the default.

    \value OvershootAlwaysOff Overshooting is never enabled (even when the
                              content is scrollable).

    \value OvershootAlwaysOn Overshooting is always enabled (even when the
                             content is not scrollable).
*/

/*!
    \enum QScrollerProperties::ScrollMetric

    This enum contains the different scroll metric types.  When not
    indicated otherwise the setScrollMetric function expects a QVariant of a
    real value.

    See the QScroller documentation for a further explanation of the
    concepts behind the different values.

    \value DragVelocitySmoothingFactor A value that describes how much new
    drag velocities are included in the final scrolling velocity.  This
    value should be in the range between \c 0 and \c 1.  Low values meaning that
    the last dragging velocity is not very important.

    \value OvershootDragResistanceFactor This value is the factor between
    the mouse dragging and the actual scroll area movement (during
    overshoot).

    \value OvershootMaximumDistanceFactor This is the maximum
    distance for overshoot movements. The actual overshoot distance will be
    calculated by multiplying this value with the viewport size of the
    scrolled object.  The factor must be in the range \c 0 to \c 1.

    \value SnapPositionRatio This is the distance that the user must drag
    the area beween two snap points in order to snap it to the next
    position. e.g. \c{0.33} means that the scroll must only reach one third
    of the distance between two snap points to snap to the next one.
    The ratio must be in the range \c 0 to \c 1.

    \value SnapTime This is the time factor for the scrolling curve.
    A lower value means that the scrolling will take longer.
    The scrolling distance is independet of this value.

    \value MousePressEventDelay This is the time a mouse press event will be
    delayed when starting a flick gesture in \c{[s]}.  If the gesture is
    triggered within that time, no mouse press or release will be sent to
    the scrolled object.  If it triggers after that delay the (delayed)
    mouse press plus a faked release event (at global postion
    \c{QPoint(-QWIDGETSIZE_MAX, -QWIDGETSIZE_MAX)} will be sent.  If the
    gesture is canceled, then both the (delayed) mouse press plus the real
    release event will be delivered.

    \value DragStartDistance This is the minimum distance the touch or
    mouse point needs to be moved before the flick gesture is triggered in \c m.

    \value MinimumVelocity The minimum velocity that is needed after ending
    the touch or releasing the mouse to start scrolling in \c{m/s}.

    \value MaximumVelocity This is the maximum velocity that can be reached
    in \c{m/s}.

    \value MaximumClickThroughVelocity This is the maximum allowed scroll
    speed for a click through in \c{m/s}.

    \value AxisLockThreshold If greater than zero a scroll movement will be
    restricted to one axis only if the movement is inside an angle about the
    axis.  The threshold must be in the range \c 0 to \c 1.

    \value AcceleratingFlickMaximumTime This is the maximum time in \c seconds
    that a flick gesture can take to be recognized as an accelerating flick. 
    If set to zero no such gesture will be detected.

    \value AcceleratingFlickSpeedupFactor The current speed will be
    multiplied by this number if an accelerating flick is detected. Should be \c{> 1}.

    \value ScrollingCurve The QEasingCurve used for the scrolling distance.
    Currently only OutQuad and OutExpo are supported.

    \value OvershootCurve The QEasingCurve used while overshooting.
    Currently only OutQuad and OutExpo are supported.

    \value ScrollMetricCount This is just used when enumerating the metrics.
    It is always the last entry.
*/

