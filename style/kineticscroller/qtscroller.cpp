/****************************************************************************
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

#include "qevent.h"
#include "qwidget.h"
#include "qtscroller.h"
#include "qtflickgesture_p.h"
#include "qtscroller_p.h"
#include "qtscrollerproperties.h"
#include "qtscrollerproperties_p.h"

#include "math.h"

#include <QTime>
#include <QMap>
#include <QApplication>
#include <QAbstractScrollArea>
#include <QGraphicsObject>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QDesktopWidget>
#include <QtCore/qmath.h>
#include <QtGui/qevent.h>
#include <qnumeric.h>

#include <QtDebug>

// vvv QScroller Solution only
#include "qtscrollerfilter_p.h"
#include "qtscrollevent.h"
// ^^^ QScroller Solution only


bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event);

//#define QSCROLLER_DEBUG

#ifdef QSCROLLER_DEBUG
#  define qScrollerDebug  qDebug
#else
#  define qScrollerDebug  while (false) qDebug
#endif

QDebug &operator<<(QDebug &dbg, const QtScrollerPrivate::ScrollSegment &s)
{
    dbg << "\n  Time: start:" << s.startTime << " duration:" << s.deltaTime;
    dbg << "\n  Pos: start:" << s.startPos << " delta:" << s.deltaPos;
    dbg << "\n  Curve: type:" << s.curve.type() << " max progress:" << s.maxProgress << "\n";
    return dbg;
}


// a few helper operators to make the code below a lot more readable:
// otherwise a lot of ifs would have to be multi-line to check both the x
// and y coordinate separately.

// returns true only if the abs. value of BOTH x and y are <= f
inline bool operator<=(const QPointF &p, qreal f)
{
    return (qAbs(p.x()) <= f) && (qAbs(p.y()) <= f);
}

// returns true only if the abs. value of BOTH x and y are < f
inline bool operator<(const QPointF &p, qreal f)
{
    return (qAbs(p.x()) < f) && (qAbs(p.y()) < f);
}

// returns true if the abs. value of EITHER x or y are >= f
inline bool operator>=(const QPointF &p, qreal f)
{
    return (qAbs(p.x()) >= f) || (qAbs(p.y()) >= f);
}

// returns true if the abs. value of EITHER x or y are > f
inline bool operator>(const QPointF &p, qreal f)
{
    return (qAbs(p.x()) > f) || (qAbs(p.y()) > f);
}

// returns a new point with both coordinates having the abs. value of the original one
inline QPointF qAbs(const QPointF &p)
{
    return QPointF(qAbs(p.x()), qAbs(p.y()));
}

// returns a new point with all components of p1 multiplied by the corresponding components of p2
inline QPointF operator*(const QPointF &p1, const QPointF &p2)
{
    return QPointF(p1.x() * p2.x(), p1.y() * p2.y());
}

// returns a new point with all components of p1 divided by the corresponding components of p2
inline QPointF operator/(const QPointF &p1, const QPointF &p2)
{
    return QPointF(p1.x() / p2.x(), p1.y() / p2.y());
}

inline QPointF clampToRect(const QPointF &p, const QRectF &rect)
{
    qreal x = qBound(rect.left(), p.x(), rect.right());
    qreal y = qBound(rect.top(), p.y(), rect.bottom());
    return QPointF(x, y);
}

// returns -1, 0 or +1 according to r being <0, ==0 or >0
inline int qSign(qreal r)
{
    return (r < 0) ? -1 : ((r > 0) ? 1 : 0);
}

// this version is not mathematically exact, but it just works for every
// easing curve type (even custom ones)

static qreal differentialForProgress(const QEasingCurve &curve, qreal pos)
{
    const qreal dx = 0.01;
    qreal left = (pos < qreal(0.5)) ? pos : pos - qreal(dx);
    qreal right = (pos >= qreal(0.5)) ? pos : pos + qreal(dx);
    qreal d = (curve.valueForProgress(right) - curve.valueForProgress(left)) / qreal(dx);

    //qScrollerDebug() << "differentialForProgress(type: " << curve.type() << ", pos: " << pos << ") = " << d;

    return d;
}

// this version is not mathematically exact, but it just works for every
// easing curve type (even custom ones)

static qreal progressForValue(const QEasingCurve &curve, qreal value)
{
    if (curve.type() >= QEasingCurve::InElastic &&
        curve.type() < QEasingCurve::Custom) {
        qWarning("progressForValue(): QEasingCurves of type %d do not have an inverse, since they are not injective.", curve.type());
        return value;
    }
    if (value < qreal(0) || value > qreal(1))
        return value;

    qreal progress = value, left(0), right(1);
    for (int iterations = 6; iterations; --iterations) {
        qreal v = curve.valueForProgress(progress);
        if (v < value)
            left = progress;
        else if (v > value)
            right = progress;
        else
            break;
        progress = (left + right) / qreal(2);
    }
    return progress;
}


class QScrollTimer : public QAbstractAnimation
{
public:
    QScrollTimer(QtScrollerPrivate *_d)
        : d(_d), ignoreUpdate(false)
    { }

    int duration() const
    {
        return -1;
    }

    void start()
    {
        // QAbstractAnimation::start() will immediately call
        // updateCurrentTime(), but our state is not set correctly yet
        ignoreUpdate = true;
        QAbstractAnimation::start();
        ignoreUpdate = false;
    }

protected:
    void updateCurrentTime(int /*currentTime*/)
    {
        if (!ignoreUpdate)
            d->timerTick();
    }

private:
    QtScrollerPrivate *d;
    bool ignoreUpdate;
};

/*!
    \class QScroller
    \brief The QScroller class enables kinetic scrolling for any scrolling widget or graphics item.
    \since 4.8
    \preliminary

    With kinetic scrolling, the user can push the widget in a given
    direction and it will continue to scroll in this direction until it is
    stopped either by the user or by friction.  Aspects of inertia, friction
    and other physical concepts can be changed in order to fine-tune an
    intuitive user experience.

    The QScroller object is the object that stores the current position and
    speed of the scrolling and takes care of updated.
    QScroller can be triggered by a gesture or directly like this:

    \code
        QScroller *scroller = QScroller::scroller(sw);
        scroller->scrollTo(QPointF(100, 100));
    \endcode

    QObjects will be informed about the new scrolling state via the QScrollEvent
    and QrepareEvent.
    The scroller uses the global QAbstractAnimation timer to generate its
    QScrollEvents, which normally results in 60 scrolls per second.

    Several examples in the \c scroller examples directory show how QScroller,
    QScrollEvent and the scroller gesture can be used.

    Even though this kinetic scroller has a huge number of settings, we recommend that you leave
    them all at their default values. In case you really want to change them you can try out the \c
    plot example in the \c scroller examples directory.

    Overshooting is an additional movement (or similar indication) of the scroll
    area when the borders are reached. Usually the scroll area will move an additional
    amount showing the blank background. In other cases the whole window may move or
    a special end-of-area marker might be shown.
    The amount and behaviour of the overshoot movement can be set via the QScrollerProperties.

    The term "accelerating flick" stands for a scrolling speed up on an already scrolling list.
    In such a case the scrolling speed is multiplied by a factor.
    The user can accelerate the list further via additional accelerating flick gestures.

    The term click-through means a touch or mouse event will stop the scrolling movement
    in addition to the "normal" action. The event will not be eaten by the scroller.

    \sa QScrollEvent, QScrollPrepareEvent
*/


QMap<QObject *, QtScroller *> QtScrollerPrivate::allScrollers;
QSet<QtScroller *> QtScrollerPrivate::activeScrollers;

/*!
    Returns \c true if a QScroller object was already created for \a target; \c false otherwise.

    \sa scroller()
*/
bool QtScroller::hasScroller(QObject *target)
{
    return (QtScrollerPrivate::allScrollers.value(target));
}
 
/*!
    Returns the scroller for the given \a target.
    As long as the object exist this function will always return the same QScroller.
    If a QScroller does not exist yet for the \a target, it will implicitly be created.
    At no point will two QScrollers be active on one object.

    \sa hasScroller(), target()
*/
QtScroller *QtScroller::scroller(QObject *target)
{
    if (!target) {
        qWarning("QScroller::scroller() was called with a null target.");
        return 0;
    }

    if (QtScrollerPrivate::allScrollers.contains(target))
        return QtScrollerPrivate::allScrollers.value(target);

    QtScroller *s = new QtScroller(target);
    QtScrollerPrivate::allScrollers.insert(target, s);
    return s;
}

const QtScroller *QtScroller::scroller(const QObject *target)
{
    return scroller(const_cast<QObject*>(target));
}

/*!
    Returns an application wide list of currently active, i.e. state() !=
    QtScroller::Inactive, QtScroller objects.
    This routine is mostly useful when writing your own gesture recognizer.
*/
QList<QtScroller *> QtScroller::activeScrollers()
{
    return QtScrollerPrivate::activeScrollers.toList();
} 

/*!
    Returns the target object of this scroller.
    \sa hasScroller(), scroller()
 */
QObject *QtScroller::target() const
{
    Q_D(const QtScroller);
    return d->target;
}

/*!
    Returns the scroller properties of this scroller.
    The properties will be used by the QScroller to determine its scrolling behaviour.
    \sa setScrollerProperties()
*/
QtScrollerProperties QtScroller::scrollerProperties() const
{
    Q_D(const QtScroller);
    return d->properties;
}


/*!
    Sets the scroller properties for this scroller.
    The properties will be used by the QScroller to determine its scrolling behaviour.
*/
void QtScroller::setScrollerProperties(const QtScrollerProperties &sp)
{
    Q_D(QtScroller);
    d->properties = sp;
}


/*!
    Registers a custom scroll gesture recognizer and grabs it for the \a
    target and returns the resulting gesture type.  If \a gestureType is
    set to TouchGesture the gesture will trigger on touch events - if set to
    one of LeftMouseButtonGesture, RightMouseButtonGesture or
    MiddleMouseButtonGesture it will trigger on mouse events of the
    corresponding button.

    Only one scroll gesture can be active on a single object at the same
    time, so if you call this function twice on the same object, it will
    ungrab the existing gesture before grabbing the new one.

    Please note: To avoid nasty side-effects, all mouse events will be
    consumed while the gesture is triggered.  Since the mouse press event is
    not consumed, the gesture needs to also send a fake mouse release event
    at the global position \c{(INT_MIN, INT_MIN)} to make sure that it
    doesn't mess with the internal states of the widget that received the
    mouse press in the first place (which could e.g.  be a QPushButton
    inside a QScrollArea).
*/
Qt::GestureType QtScroller::grabGesture(QObject *target, ScrollerGestureType scrollGestureType)
{
    // ensure that a scroller for target is created
    QtScroller *s = scroller(target);
    if (!s)
        return Qt::GestureType(0);

    QtScrollerPrivate *sp = s->d_ptr;
    if (sp->recognizer)
        ungrabGesture(target); // ungrab the old gesture

    Qt::MouseButton button;
    switch (scrollGestureType) {
    case LeftMouseButtonGesture  : button = Qt::LeftButton; break;
    case RightMouseButtonGesture : button = Qt::RightButton; break;
    case MiddleMouseButtonGesture: button = Qt::MidButton; break;
    case TouchGesture            : button = Qt::NoButton; break; // NoButton == Touch
    }

    sp->recognizer = new QtFlickGestureRecognizer(button);
    sp->recognizerType = QGestureRecognizer::registerRecognizer(sp->recognizer);

    if (target->isWidgetType()) {
        QWidget *widget = static_cast<QWidget *>(target);
        widget->grabGesture(sp->recognizerType);
        if (scrollGestureType == TouchGesture)
            widget->setAttribute(Qt::WA_AcceptTouchEvents);

    } else if(QGraphicsObject *go = qobject_cast<QGraphicsObject*>(target)) {
        if (scrollGestureType == TouchGesture)
            go->setAcceptTouchEvents(true);
        go->grabGesture(sp->recognizerType);
    }
    
    // vvv QScroller Solution only
    target->installEventFilter(QtScrollerFilter::instance());
    // ^^^ QScroller Solution only
    
    return sp->recognizerType;
}

/*!
    Returns the gesture type currently grabbed for the \a target or 0 if no
    gesture is grabbed.
*/
Qt::GestureType QtScroller::grabbedGesture(QObject *target)
{
    QtScroller *s = scroller(target);
    if (s && s->d_ptr)
        return s->d_ptr->recognizerType;
    else
        return Qt::GestureType(0);
}

/*!
    Ungrabs the gesture for the \a target.
*/
void QtScroller::ungrabGesture(QObject *target)
{
    QtScroller *s = scroller(target);
    if (!s)
        return;

    QtScrollerPrivate *sp = s->d_ptr;
    if (!sp->recognizer)
        return; // nothing to do

    if (target->isWidgetType()) {
        QWidget *widget = static_cast<QWidget *>(target);
        widget->ungrabGesture(sp->recognizerType);

    } else if(QGraphicsObject *go = qobject_cast<QGraphicsObject*>(target)) {
        go->ungrabGesture(sp->recognizerType);
    }

    QGestureRecognizer::unregisterRecognizer(sp->recognizerType);
    // do not delete the recognizer. The QGestureManager is doing this.
    sp->recognizer = 0;

    // vvv QScroller Solution only
    target->removeEventFilter(QtScrollerFilter::instance());
    // ^^^ QScroller Solution only
}

/*!
    \internal
*/
QtScroller::QtScroller(QObject *target)
    : d_ptr(new QtScrollerPrivate(this, target))
{
    Q_ASSERT(target); // you can't create a scroller without a target in any normal way
    Q_D(QtScroller);
    d->init();
}

/*!
    \internal
*/
QtScroller::~QtScroller()
{
    Q_D(QtScroller);
    QGestureRecognizer::unregisterRecognizer(d->recognizerType);
    // do not delete the recognizer. The QGestureManager is doing this.
    d->recognizer = 0;
    QtScrollerPrivate::allScrollers.remove(d->target);
    QtScrollerPrivate::activeScrollers.remove(this);

    delete d_ptr;
}

/*!
    \property QScroller::state
    \brief the state of the scroller

    \sa QScroller::State
*/

QtScroller::State QtScroller::state() const
{
    Q_D(const QtScroller);
    return d->state;
}

/*!
    Stops the scroller and resets the state back to Inactive.
*/
void QtScroller::stop()
{
    Q_D(QtScroller);
    if (d->state != Inactive) {
        QPointF here = clampToRect(d->contentPosition, d->contentPosRange);
        qreal snapX = d->nextSnapPos(here.x(), 0, Qt::Horizontal);
        qreal snapY = d->nextSnapPos(here.y(), 0, Qt::Vertical);
        QPointF snap = here;
        if (!qIsNaN(snapX))
            snap.setX(snapX);
        if (!qIsNaN(snapY))
            snap.setY(snapY);
        d->contentPosition = snap;
        d->overshootPosition = QPointF(0, 0);

        d->setState(Inactive);
    }
}

/*!
    \brief Returns the pixel per meter metric for the scrolled widget.

    The value is reported for both the x and y axis separately by using a QPointF.

    \note Please note that this value should be physically correct, while the actual DPI settings
    that Qt returns for the display may be reported wrongly (on purpose) by the underlying
    windowing system (e.g. Mac OS X or Maemo 5).
*/
QPointF QtScroller::pixelPerMeter() const
{
    Q_D(const QtScroller);
    QPointF ppm = d->pixelPerMeter;

    if (QGraphicsObject *go = qobject_cast<QGraphicsObject *>(d->target)) {
        QTransform viewtr;
        //TODO: the first view isn't really correct - maybe use an additional field in the prepare event?
        if (go->scene() && !go->scene()->views().isEmpty())
            viewtr = go->scene()->views().first()->viewportTransform();
        QTransform tr = go->deviceTransform(viewtr);
        if (tr.isScaling()) {
            QPointF p0 = tr.map(QPointF(0, 0));
            QPointF px = tr.map(QPointF(1, 0));
            QPointF py = tr.map(QPointF(0, 1));
            ppm.rx() /= QLineF(p0, px).length();
            ppm.ry() /= QLineF(p0, py).length();
        }
    }
    return ppm;
}

/*!
    \brief Returns the current velocity of the scroller.

    Returns the current scrolling velocity in meter per second when in the state Scrolling.
    Returns a null velocity otherwise.

    The velocity is reported for both the x and y axis separately by using a QPointF.

    \sa pixelPerMeter
*/
QPointF QtScroller::velocity() const
{
    Q_D(const QtScroller);
    const QtScrollerPropertiesPrivate *sp = d->properties.d.data();

     switch (state()) {
     case Dragging:
        return d->releaseVelocity;
     case Scrolling: {
        QPointF vel;
        qint64 now = d->monotonicTimer.elapsed();

        if (!d->xSegments.isEmpty()) {
            const QtScrollerPrivate::ScrollSegment &s = d->xSegments.head();
            qreal progress = qreal(now - s.startTime) / (qreal(s.deltaTime) / s.maxProgress);
            qreal v = qSign(s.deltaPos) * qreal(s.deltaTime) / qreal(1000) * sp->decelerationFactor * qreal(0.5) * differentialForProgress(s.curve, progress);
            vel.setX(v);
        }

        if (!d->ySegments.isEmpty()) {
            const QtScrollerPrivate::ScrollSegment &s = d->ySegments.head();
            qreal progress = qreal(now - s.startTime) / (qreal(s.deltaTime) / s.maxProgress);
            qreal v = qSign(s.deltaPos) * qreal(s.deltaTime) / qreal(1000) * sp->decelerationFactor * qreal(0.5) * differentialForProgress(s.curve, progress);
            vel.setY(v);
        }
        //qScrollerDebug() << "Velocity: " << vel;
        return vel;
     }
     default:
         return QPointF(0, 0);
     }
}

/*!
    Starts scrolling the widget so that point \a pos is at the top-left position in
    the viewport.

    The behaviour when scrolling outside the valid scroll area is undefined.
    In this case the scroller might or might not overshoot.

    The scrolling speed will be calculated so that the given position will
    be reached after a platform-defined time span (e.g. 1 second for Maemo 5).

    \a pos is given in viewport coordinates, while the \a scrollTime is given in milli seconds.

    \sa ensureVisible()
*/
void QtScroller::scrollTo(const QPointF &pos, int scrollTime)
{
    Q_D(QtScroller);

    if (d->state == Pressed || d->state == Dragging )
        return;

    // no need to resend a prepare event if we are already scrolling
    if (d->state == Inactive && !d->prepareScrolling(QPointF()))
        return;

    QPointF newpos = clampToRect(pos, d->contentPosRange);
    qreal snapX = d->nextSnapPos(newpos.x(), 0, Qt::Horizontal);
    qreal snapY = d->nextSnapPos(newpos.y(), 0, Qt::Vertical);
    if (!qIsNaN(snapX))
        newpos.setX(snapX);
    if (!qIsNaN(snapY))
        newpos.setY(snapY);

    qScrollerDebug() << "QtScroller::scrollTo(req:" << pos << " [pix] / snap:" << newpos << ", " << scrollTime << " [ms])";

    if (newpos == d->contentPosition + d->overshootPosition)
        return;

    QPointF vel = velocity();

    if (scrollTime < 0)
        scrollTime = 0;
    qreal time = qreal(scrollTime) / 1000;

    d->createScrollToSegments(vel.x(), time, newpos.x(), Qt::Horizontal, QtScrollerPrivate::ScrollTypeScrollTo);
    d->createScrollToSegments(vel.y(), time, newpos.y(), Qt::Vertical, QtScrollerPrivate::ScrollTypeScrollTo);

    d->setState(Scrolling);
}

/*!
    Starts scrolling so that the rectangle \a rect is visible inside the
    viewport with additional margins specified in pixels by \a xmargin and \a ymargin around
    the rect.

    In cases where it is not possible to fit the rect plus margins inside the viewport the contents
    are scrolled so that as much as possible is visible from \a rect.

    This function performs the actual scrolling by calling scrollTo().
*/
void QtScroller::ensureVisible(const QRectF &rect, qreal xmargin, qreal ymargin, int scrollTime)
{
    Q_D(QtScroller);

    if (d->state == Pressed || d->state == Dragging )
        return;

    if (d->state == Inactive || !d->prepareScrolling(QPointF()))
        return;

    // -- calculate the current pos (or the position after the current scroll)
    QPointF startPos = d->contentPosition + d->overshootPosition;
    startPos = QPointF(d->scrollingSegmentsEndPos(Qt::Horizontal),
                       d->scrollingSegmentsEndPos(Qt::Vertical));

    QRectF marginRect(rect.x() - xmargin, rect.y() - ymargin,
                      rect.width() + 2 * xmargin, rect.height() + 2 * ymargin);
 
    QSizeF visible = d->viewportSize;
    QRectF visibleRect(startPos, visible);

    qScrollerDebug() << "QScroller::ensureVisible(" << rect << " [pix], " << xmargin << " [pix], " << ymargin << " [pix], " << scrollTime << "[ms])";
    qScrollerDebug() << "  --> content position:" << d->contentPosition;

    if (visibleRect.contains(marginRect))
        return;

    QPointF newPos = startPos;
  
    if (visibleRect.width() < rect.width()) {
        // at least try to move the rect into view
        if (rect.left() > visibleRect.left())
            newPos.setX(rect.left());
        else if (rect.right() < visibleRect.right()) 
            newPos.setX(rect.right() - visible.width());

    } else if (visibleRect.width() < marginRect.width()) {
        newPos.setX(rect.center().x() - visibleRect.width() / 2);
    } else if (marginRect.left() > visibleRect.left()) {
        newPos.setX(marginRect.left());
    } else if (marginRect.right() < visibleRect.right()) {
        newPos.setX(marginRect.right() - visible.width());
    }

    if (visibleRect.height() < rect.height()) {
        // at least try to move the rect into view
        if (rect.top() > visibleRect.top())
            newPos.setX(rect.top()); 
        else if (rect.bottom() < visibleRect.bottom())
            newPos.setX(rect.bottom() - visible.height());

    } else if (visibleRect.height() < marginRect.height()) {
        newPos.setY(rect.center().y() - visibleRect.height() / 2);
    } else if (marginRect.top() > visibleRect.top()) {
        newPos.setY(marginRect.top());
    } else if (marginRect.bottom() < visibleRect.bottom()) {
        newPos.setY(marginRect.bottom() - visible.height());
    }

    // clamp to maximum content position
    newPos = clampToRect(newPos, d->contentPosRange);
    if (newPos == startPos)
        return;

    scrollTo(newPos, scrollTime);
}

/*! This function resends the QScrollPrepareEvent.
 *  Calling resendPrepareEvent triggers a QScrollPrepareEvent from the scroller.
 *  This will allow the receiver to re-set content position and content size while
 *  scrolling.
 *  Calling this function while in the Inactive state is useless as the prepare event
 *  is send again right before scrolling starts.
 */
void QtScroller::resendPrepareEvent()
{
    Q_D(QtScroller);
    d->prepareScrolling(d->pressPosition);
}

/*! Set the snap positions for the horizontal axis.
 *  Set the snap positions to a list of positions.
 *  This will overwrite all previously set snap positions and also a previously
 *  set snapping interval.
 *  Snapping can be deactivated by setting an empty list of positions.
 */
void QtScroller::setSnapPositionsX(const QList<qreal> &positions)
{
    Q_D(QtScroller);
    d->snapPositionsX = positions;
    d->snapIntervalX = 0.0;

    d->recalcScrollingSegments();
}

/*! Set the snap positions for the horizontal axis.
 *  Set the snap positions to regular spaced intervals.
 *  The first snap position will be at \a first. The next at \a first + \a interval and so on.
 *  This will overwrite all previously set snap positions and also a previously
 *  set snapping interval.
 *  Snapping can be deactivated by setting an interval of 0.0
 */
void QtScroller::setSnapPositionsX(qreal first, qreal interval)
{
    Q_D(QtScroller);
    d->snapFirstX = first;
    d->snapIntervalX = interval;
    d->snapPositionsX.clear();

    d->recalcScrollingSegments();
}

/*! Set the snap positions for the vertical axis.
 *  Set the snap positions to a list of positions.
 *  This will overwrite all previously set snap positions and also a previously
 *  set snapping interval.
 *  Snapping can be deactivated by setting an empty list of positions.
 */
void QtScroller::setSnapPositionsY(const QList<qreal> &positions)
{
    Q_D(QtScroller);
    d->snapPositionsY = positions;
    d->snapIntervalY = 0.0;

    d->recalcScrollingSegments();
}

/*! Set the snap positions for the vertical axis.
 *  Set the snap positions to regular spaced intervals.
 *  The first snap position will be at \a first. The next at \a first + \a interval and so on.
 *  This will overwrite all previously set snap positions and also a previously
 *  set snapping interval.
 *  Snapping can be deactivated by setting an interval of 0.0
 */
void QtScroller::setSnapPositionsY( qreal first, qreal interval )
{
    Q_D(QtScroller);
    d->snapFirstY = first;
    d->snapIntervalY = interval;
    d->snapPositionsY.clear();

    d->recalcScrollingSegments();
}



// -------------- private ------------

QtScrollerPrivate::QtScrollerPrivate(QtScroller *q, QObject *_target)
    : target(_target)
    , recognizer(0)
    , recognizerType(Qt::CustomGesture)
    , state(QtScroller::Inactive)
    , firstScroll(true)
    , pressTimestamp(0)
    , lastTimestamp(0)
    , snapFirstX(-1.0)
    , snapIntervalX(0.0)
    , snapFirstY(-1.0)
    , snapIntervalY(0.0)
    , scrollTimer(new QScrollTimer(this))
    , q_ptr(q)
{
    connect(target, SIGNAL(destroyed(QObject*)), this, SLOT(targetDestroyed()));
}

void QtScrollerPrivate::init()
{
    setDpiFromWidget(0);
    monotonicTimer.start();
}

void QtScrollerPrivate::sendEvent(QObject *o, QEvent *e)
{
    qt_sendSpontaneousEvent(o, e);
}

const char *QtScrollerPrivate::stateName(QtScroller::State state)
{
    switch (state) {
    case QtScroller::Inactive:  return "inactive";
    case QtScroller::Pressed:   return "pressed";
    case QtScroller::Dragging:  return "dragging";
    case QtScroller::Scrolling: return "scrolling";
    default:                        return "(invalid)";
    }
}

const char *QtScrollerPrivate::inputName(QtScroller::Input input)
{
    switch (input) {
    case QtScroller::InputPress:   return "press";
    case QtScroller::InputMove:    return "move";
    case QtScroller::InputRelease: return "release";
    default:                      return "(invalid)";
    }
}

void QtScrollerPrivate::targetDestroyed()
{
    scrollTimer->stop();
    delete q_ptr;
}

void QtScrollerPrivate::timerTick()
{
    struct timerevent {
        QtScroller::State state;
        typedef void (QtScrollerPrivate::*timerhandler_t)();
        timerhandler_t handler;
    };

    timerevent timerevents[] = {
        { QtScroller::Dragging, &QtScrollerPrivate::timerEventWhileDragging },
        { QtScroller::Scrolling, &QtScrollerPrivate::timerEventWhileScrolling },
    };

    for (int i = 0; i < int(sizeof(timerevents) / sizeof(*timerevents)); ++i) {
        timerevent *te = timerevents + i;

        if (state == te->state) {
            (this->*te->handler)();
            return;
        }
    }

    scrollTimer->stop();
}

/*!
    This function is used by gesture recognizers to inform the scroller about a new input event.
    The scroller will change it's internal state() according to the input event its current state
    and its scrollerProperties().

    \note Using grabGesture() should be sufficient for most use cases
    though.
*/
bool QtScroller::handleInput(Input input, const QPointF &position, qint64 timestamp)
{
    Q_D(QtScroller);

    qScrollerDebug() << "QScroller::handleInput(" << input << ", " << d->stateName(d->state) << ", " << position << ", " << timestamp << ")";
    struct statechange {
        State state;
        Input input;
        typedef bool (QtScrollerPrivate::*inputhandler_t)(const QPointF &position, qint64 timestamp);
        inputhandler_t handler;
    };

    statechange statechanges[] = {
        { QtScroller::Inactive,  InputPress,   &QtScrollerPrivate::pressWhileInactive },
        { QtScroller::Pressed,   InputMove,    &QtScrollerPrivate::moveWhilePressed },
        { QtScroller::Pressed,   InputRelease, &QtScrollerPrivate::releaseWhilePressed },
        { QtScroller::Dragging,  InputMove,    &QtScrollerPrivate::moveWhileDragging },
        { QtScroller::Dragging,  InputRelease, &QtScrollerPrivate::releaseWhileDragging },
        { QtScroller::Scrolling, InputPress,   &QtScrollerPrivate::pressWhileScrolling }
    };

    for (int i = 0; i < int(sizeof(statechanges) / sizeof(*statechanges)); ++i) {
        statechange *sc = statechanges + i;

         if (d->state == sc->state && input == sc->input)
             return (d->*sc->handler)(position - d->overshootPosition, timestamp);
    }
    return false;
}

#ifdef Q_WS_MAEMO_5

QPointF QScrollerPrivate::realDpi(int screen)
{
    Q_UNUSED(screen);
    // The DPI value is hardcoded to 96 on Maemo5:
    // https://projects.maemo.org/bugzilla/show_bug.cgi?id=152525
    // This value (260) is only correct for the N900 though, but
    // there's no way to get the real DPI at run time.
    return QPointF(260, 260);
}

#elif defined(Q_WS_MAC)

// implemented in qscroller_mac.mm

#else

QPointF QtScrollerPrivate::realDpi(int screen)
{
    QWidget *w = QApplication::desktop()->screen(screen);
    return QPointF(w->physicalDpiX(), w->physicalDpiY());
}

#endif

/*! \internal
    Returns the resolution of the used screen.
*/
QPointF QtScrollerPrivate::dpi() const
{
    return pixelPerMeter / qreal(39.3700787);
}

/*! \internal
    Sets the resolution used for scrolling.
    This resolution is only used by the kinetic scroller. If you change this
    then the scroller will behave quite different as a lot of the values are
    given in physical distances (millimeter).
*/
void QtScrollerPrivate::setDpi(const QPointF &dpi)
{
    pixelPerMeter = dpi * qreal(39.3700787);
}

/*! \internal
    Sets the dpi used for scrolling to the value of the widget.
*/
void QtScrollerPrivate::setDpiFromWidget(QWidget *widget)
{
    QDesktopWidget *dw = QApplication::desktop();
    setDpi(realDpi(widget ? dw->screenNumber(widget) : dw->primaryScreen()));
}

/*! \internal
    Updates the velocity during dragging.
    Sets releaseVelocity.
*/
void QtScrollerPrivate::updateVelocity(const QPointF &deltaPixelRaw, qint64 deltaTime)
{
    Q_Q(QtScroller);
    QPointF ppm = q->pixelPerMeter();
    const QtScrollerPropertiesPrivate *sp = properties.d.data();
    QPointF deltaPixel = deltaPixelRaw;

    qScrollerDebug() << "QScroller::updateVelocity(" << deltaPixelRaw << " [delta pix], " << deltaTime << " [delta ms])";

    // faster than 2.5mm/ms seems bogus (that would be a screen height in ~20 ms)
    if (((deltaPixelRaw / qreal(deltaTime)).manhattanLength() / ((ppm.x() + ppm.y()) / 2) * 1000) > qreal(2.5))
        deltaPixel = deltaPixelRaw * qreal(2.5) * ppm / 1000 / (deltaPixelRaw / qreal(deltaTime)).manhattanLength();

    qreal inversSmoothingFactor = ((qreal(1) - sp->dragVelocitySmoothingFactor) * qreal(deltaTime) / qreal(1000));
    QPointF newv = -deltaPixel / qreal(deltaTime) * qreal(1000) / ppm;
    newv = newv * (qreal(1) - inversSmoothingFactor) + releaseVelocity * inversSmoothingFactor;

    if (deltaPixel.x())
        releaseVelocity.setX(qBound(-sp->maximumVelocity, newv.x(), sp->maximumVelocity));
    if (deltaPixel.y())
        releaseVelocity.setY(qBound(-sp->maximumVelocity, newv.y(), sp->maximumVelocity));

    qScrollerDebug() << "  --> new velocity:" << releaseVelocity;
}

void QtScrollerPrivate::pushSegment(ScrollType type, qreal deltaTime, qreal deltaPos, QEasingCurve::Type curve, Qt::Orientation orientation, qreal maxProgress)
{
    if (deltaPos == 0.0)
        return;

    ScrollSegment s;
    if (orientation == Qt::Horizontal && !xSegments.isEmpty()) {
        s.startTime = xSegments.last().startTime + xSegments.last().deltaTime;
        s.startPos = xSegments.last().startPos + xSegments.last().deltaPos;
    } else if (orientation == Qt::Vertical && !ySegments.isEmpty()) {
        s.startTime = ySegments.last().startTime + ySegments.last().deltaTime;
        s.startPos = ySegments.last().startPos + ySegments.last().deltaPos;
    } else {
        qint64 time = monotonicTimer.elapsed();
        s.startTime = time;
        if (orientation == Qt::Horizontal)
            s.startPos = contentPosition.x() + overshootPosition.x();
        else
            s.startPos = contentPosition.y() + overshootPosition.y();
    }
    s.deltaTime = deltaTime * 1000;
    s.deltaPos = deltaPos;
    s.maxProgress = maxProgress;
    s.curve.setType(curve);
    s.type = type;

    if (orientation == Qt::Horizontal)
        xSegments.enqueue(s);
    else
        ySegments.enqueue(s);

    qScrollerDebug() << "+++ Added a new ScrollSegment: " << s;
}


/*! \internal
    Clears the old segments and recalculates them if the current segments are not longer valid
*/
void QtScrollerPrivate::recalcScrollingSegments()
{
    Q_Q(QtScroller);
    QPointF ppm = q->pixelPerMeter();

    releaseVelocity = q->velocity();

    if (!scrollingSegmentsValid(Qt::Horizontal))
        createScrollingSegments(releaseVelocity.x(), contentPosition.x() + overshootPosition.x(), ppm.x(), Qt::Horizontal);

    if (!scrollingSegmentsValid(Qt::Vertical))
        createScrollingSegments(releaseVelocity.y(), contentPosition.y() + overshootPosition.y(), ppm.y(), Qt::Vertical);
}

/*! \internal
    Returns the end position after the current scroll has finished.
*/
qreal QtScrollerPrivate::scrollingSegmentsEndPos(Qt::Orientation orientation)
{
    QQueue<ScrollSegment> *segments;
    qreal endPos;
 
    if (orientation == Qt::Horizontal) {
        segments = &xSegments;
        endPos = contentPosition.x() + overshootPosition.x();
    } else {
        segments = &ySegments;
        endPos = contentPosition.y() + overshootPosition.y();
    }

    if (!segments->isEmpty()) {
        const ScrollSegment &last = segments->last();
        endPos = last.startPos + last.deltaPos;
    }

    return endPos;
}

/*! \internal
    Checks if the scroller segment end in a valid position.
*/
bool QtScrollerPrivate::scrollingSegmentsValid(Qt::Orientation orientation)
{
    QQueue<ScrollSegment> *segments;
    qreal minPos;
    qreal maxPos;

    if (orientation == Qt::Horizontal) {
        segments = &xSegments;
        minPos = contentPosRange.left();
        maxPos = contentPosRange.right();
    } else {
        segments = &ySegments;
        minPos = contentPosRange.top();
        maxPos = contentPosRange.bottom();
    }

    if (segments->isEmpty())
        return true;

    const ScrollSegment &last = segments->last();
    qreal endPos = last.startPos + last.deltaPos;

    if (last.type == ScrollTypeScrollTo)
        return true; // scrollTo is always valid

    if (last.type == ScrollTypeOvershoot &&
        endPos != minPos && endPos != maxPos)
        return false;

    if (endPos < minPos || endPos > maxPos)
        return false;

    if (endPos == minPos || endPos == maxPos) // the begin and the end of the list are always ok
        return true;

    qreal nextSnap = nextSnapPos(endPos, 0, orientation);
    if (!qIsNaN(nextSnap) && endPos != nextSnap)
        return false;

    return true;
}

/*! \internal
   Creates the sections needed to scroll to the specific \endPos to the segments queue.
*/
void QtScrollerPrivate::createScrollToSegments(qreal v, qreal deltaTime, qreal endPos, Qt::Orientation orientation, ScrollType type)
{
    Q_UNUSED(v);

    if (orientation == Qt::Horizontal) {
        xSegments.clear();
    } else {
        ySegments.clear();
    }

    qScrollerDebug() << "+++ createScrollToSegments: t:" << deltaTime << "ep:" << endPos << "o:" << int(orientation);

    const QtScrollerPropertiesPrivate *sp = properties.d.data();

    qreal startPos = (orientation == Qt::Horizontal) ? contentPosition.x() + overshootPosition.x()
                                                     : contentPosition.y() + overshootPosition.y();
    qreal deltaPos = endPos - startPos;

    pushSegment(type, deltaTime * 0.3, deltaPos * 0.5, QEasingCurve::InQuad, orientation);
    pushSegment(type, deltaTime * 0.7, deltaPos - deltaPos * 0.5, sp->scrollingCurve.type(), orientation);
}

/*! \internal
*/
void QtScrollerPrivate::createScrollingSegments(qreal v, qreal startPos, qreal ppm, Qt::Orientation orientation)
{
    const QtScrollerPropertiesPrivate *sp = properties.d.data();

    QtScrollerProperties::OvershootPolicy policy;
    qreal minPos;
    qreal maxPos;
    qreal viewSize;

    if (orientation == Qt::Horizontal) {
        xSegments.clear();
        policy = sp->hOvershootPolicy;
        minPos = contentPosRange.left();
        maxPos = contentPosRange.right();
        viewSize = viewportSize.width();
    } else {
        ySegments.clear();
        policy = sp->vOvershootPolicy;
        minPos = contentPosRange.top();
        maxPos = contentPosRange.bottom();
        viewSize = viewportSize.height();
    }

    bool alwaysOvershoot = (policy == QtScrollerProperties::OvershootAlwaysOn);
    bool noOvershoot = (policy == QtScrollerProperties::OvershootAlwaysOff) || !sp->overshootMaximumDistanceFactor;
    bool canOvershoot = !noOvershoot && (alwaysOvershoot || maxPos);

    qScrollerDebug() << "+++ createScrollingSegments: s:" << startPos << "maxPos:" << maxPos << "o:" << int(orientation);

    // - check if we are in overshoot
    if (startPos < minPos) {
        createScrollToSegments(v, sp->overshootTime *0.5, 0, orientation, ScrollTypeOvershoot);
        return;
    }

    if (startPos > maxPos) {
        createScrollToSegments(v, sp->overshootTime * 0.5, maxPos, orientation, ScrollTypeOvershoot);
        return;
    }

    qScrollerDebug() << "v = " << v << ", decelerationFactor = " << sp->decelerationFactor << ", curveType = " << sp->scrollingCurve.type();

    // This is only correct for QEasingCurve::OutQuad (linear velocity,
    // constant deceleration), but the results look and feel ok for OutExpo
    // and OutSine as well

    // v(t) = deltaTime * a * 0.5 * differentialForProgress(t / deltaTime)
    // v(0) = vrelease
    // v(deltaTime) = 0
    // deltaTime = (2 * vrelease) / (a * differntial(0))

    // pos(t) = integrate(v(t)dt)
    // pos(t) = vrelease * t - 0.5 * a * t * t
    // pos(t) = deltaTime * a * 0.5 * progress(t / deltaTime) * deltaTime
    // deltaPos = pos(deltaTime)

    qreal deltaTime = (qreal(2) * qAbs(v)) / (sp->decelerationFactor * differentialForProgress(sp->scrollingCurve, 0));
    qreal deltaPos = qSign(v) * deltaTime * deltaTime * qreal(0.5) * sp->decelerationFactor * ppm;
    qreal endPos = startPos + deltaPos;

    qScrollerDebug() << "Real Delta:" << deltaPos;

    // - check if we are overshooting
    if (endPos < minPos || endPos > maxPos) {
        qreal stopPos = endPos < minPos ? minPos : maxPos;

        qScrollerDebug() << "Overshoot: delta:" << (stopPos - startPos);

        qreal maxProgress = progressForValue(sp->scrollingCurve, qAbs((stopPos - startPos) / deltaPos));
        qScrollerDebug() << "Overshoot maxp:" << maxProgress;

        pushSegment(ScrollTypeOvershoot, deltaTime * maxProgress, stopPos - startPos, sp->scrollingCurve.type(), orientation, maxProgress);

        if (canOvershoot) {
            qreal endV = qSign(v) * deltaTime * sp->decelerationFactor * qreal(0.5) * differentialForProgress(sp->scrollingCurve, maxProgress);
            qScrollerDebug() << "Overshoot: velocity" << endV;
            qScrollerDebug() << "Overshoot: maxVelocity" << sp->maximumVelocity;
            qScrollerDebug() << "Overshoot: viewsize" << viewSize;
            qScrollerDebug() << "Overshoot: factor" << sp->overshootMaximumDistanceFactor;

            qreal oDistance = viewSize * sp->overshootMaximumDistanceFactor * endV / sp->maximumVelocity;
            qreal oDeltaTime = sp->overshootTime;

            pushSegment(ScrollTypeOvershoot, oDeltaTime * 0.5, oDistance, sp->overshootCurve.type(), orientation);
            pushSegment(ScrollTypeOvershoot, oDeltaTime * 0.3, -oDistance * 0.7, QEasingCurve::InQuad, orientation);
            pushSegment(ScrollTypeOvershoot, oDeltaTime * 0.2, -oDistance + oDistance * 0.7, QEasingCurve::OutQuad, orientation);
        }
        return;
    }

    qreal nextSnap = nextSnapPos(endPos, 0, orientation);
    qreal lowerSnapPos = nextSnapPos(startPos, -1, orientation);
    qreal higherSnapPos = nextSnapPos(startPos, 1, orientation);

    // - check if we can reach another snap point
    if (nextSnap > higherSnapPos || qIsNaN(higherSnapPos))
        higherSnapPos = nextSnap;
    if (nextSnap > lowerSnapPos || qIsNaN(lowerSnapPos))
        lowerSnapPos = nextSnap;

    if (qAbs(v) < sp->minimumVelocity) {

        qScrollerDebug() << "### below minimum Vel" << orientation;

        // - no snap points or already at one
        if (qIsNaN(nextSnap) || nextSnap == startPos)
            return; // nothing to do, no scrolling needed.

        // - decide which point to use

        qreal snapDistance = higherSnapPos - lowerSnapPos;

        qreal pressDistance = (orientation == Qt::Horizontal) ?
            lastPosition.x() - pressPosition.x() :
            lastPosition.y() - pressPosition.y();

        // if not dragged far enough, pick the next snap point.
        if (sp->snapPositionRatio == 0.0 || qAbs(pressDistance / sp->snapPositionRatio) > snapDistance)
            endPos = nextSnap;
        else if (pressDistance < 0.0)
            endPos = lowerSnapPos;
        else
            endPos = higherSnapPos;

        deltaPos = endPos - startPos;
        pushSegment(ScrollTypeFlick, sp->snapTime * 0.3, deltaPos * 0.3, QEasingCurve::InQuad, orientation);
        pushSegment(ScrollTypeFlick, sp->snapTime * 0.7, deltaPos - deltaPos * 0.3, sp->scrollingCurve.type(), orientation);
        return;
    }

    // - go to the next snappoint if there is one
    if (!qIsNaN(nextSnap)) {
        if (v > 0)
            deltaPos = higherSnapPos - startPos;
        else if (v < 0)
            deltaPos = lowerSnapPos - startPos;
    }

    pushSegment(ScrollTypeFlick, deltaTime, deltaPos, sp->scrollingCurve.type(), orientation);
}


/*! Prepares scrolling by sending a QScrollPrepareEvent to the receiver widget.
    Returns true if the scrolling was accepted and a target was returned.
*/
bool QtScrollerPrivate::prepareScrolling(const QPointF &position)
{
    QtScrollPrepareEvent spe(position);
    spe.ignore();
    sendEvent(target, &spe);

    qScrollerDebug() << "QScrollPrepareEvent returned from" << target << "with" << spe.isAccepted() << "mcp:" << spe.contentPosRange() << "cp:" << spe.contentPos();
    if (spe.isAccepted()) {
        QPointF oldContentPos = contentPosition + overshootPosition;
        QPointF contentDelta = spe.contentPos() - oldContentPos;

        viewportSize = spe.viewportSize();
        contentPosRange = spe.contentPosRange();
        if (contentPosRange.width() < 0)
            contentPosRange.setWidth(0);
        if (contentPosRange.height() < 0)
            contentPosRange.setHeight(0);
        contentPosition = clampToRect(spe.contentPos(), contentPosRange);
        overshootPosition = spe.contentPos() - contentPosition;

        // - check if the content position was moved
        if (!contentDelta.isNull()) {
            // need to correct all segments
            for (int i = 0; i < xSegments.count(); i++)
                xSegments[i].startPos -= contentDelta.x();

            for (int i = 0; i < ySegments.count(); i++)
                ySegments[i].startPos -= contentDelta.y();
        }

        if (QWidget *w = qobject_cast<QWidget *>(target))
            setDpiFromWidget(w);
        if (QGraphicsObject *go = qobject_cast<QGraphicsObject *>(target)) {
            //TODO: the first view isn't really correct - maybe use an additional field in the prepare event?
            if (go->scene() && !go->scene()->views().isEmpty())
                setDpiFromWidget(go->scene()->views().first());
        }

        if (state == QtScroller::Scrolling) {
            recalcScrollingSegments();
        }
        return true;
    }

    return false;
}

void QtScrollerPrivate::handleDrag(const QPointF &position, qint64 timestamp)
{
    const QtScrollerPropertiesPrivate *sp = properties.d.data();

    QPointF deltaPixel = position - lastPosition;
    qint64 deltaTime = timestamp - lastTimestamp;

    if (sp->axisLockThreshold) {
        int dx = qAbs(deltaPixel.x());
        int dy = qAbs(deltaPixel.y());
        if (dx || dy) {
            bool vertical = (dy > dx);
            qreal alpha = qreal(vertical ? dx : dy) / qreal(vertical ? dy : dx);
            //qScrollerDebug() << "QScroller::handleDrag() -- axis lock:" << alpha << " / " << axisLockThreshold << "- isvertical:" << vertical << "- dx:" << dx << "- dy:" << dy;
            if (alpha <= sp->axisLockThreshold) {
                if (vertical)
                    deltaPixel.setX(0);
                else
                    deltaPixel.setY(0);
            }
        }
    }

    // calculate velocity (if the user would release the mouse NOW)
    updateVelocity(deltaPixel, deltaTime);

    // restrict velocity, if content is not scrollable
    QRectF max = contentPosRange;
    bool canScrollX = (max.width() > 0) || (sp->hOvershootPolicy == QtScrollerProperties::OvershootAlwaysOn);
    bool canScrollY = (max.height() > 0) || (sp->vOvershootPolicy == QtScrollerProperties::OvershootAlwaysOn);

    if (!canScrollX) {
        deltaPixel.setX(0);
        releaseVelocity.setX(0);
    }
    if (!canScrollY) {
        deltaPixel.setY(0);
        releaseVelocity.setY(0);
    }

//    if (firstDrag) {
//        // Do not delay the first drag
//        setContentPositionHelper(q->contentPosition() - overshootDistance - deltaPixel);
//        dragDistance = QPointF(0, 0);
//    } else {
    dragDistance += deltaPixel;
//    }
//qScrollerDebug() << "######################" << deltaPixel << position.y() << lastPosition.y();
    if (canScrollX)
        lastPosition.setX(position.x());
    if (canScrollY)
        lastPosition.setY(position.y());
    lastTimestamp = timestamp;
}

bool QtScrollerPrivate::pressWhileInactive(const QPointF &position, qint64 timestamp)
{
    if (prepareScrolling(position)) {
        const QtScrollerPropertiesPrivate *sp = properties.d.data();

        if (!contentPosRange.size().isNull() ||
            (sp->hOvershootPolicy == QtScrollerProperties::OvershootAlwaysOn) ||
            (sp->vOvershootPolicy == QtScrollerProperties::OvershootAlwaysOn)) {

            lastPosition = pressPosition = position;
            lastTimestamp = pressTimestamp = timestamp;
            setState(QtScroller::Pressed);
        }
    }
    return false;
}

bool QtScrollerPrivate::releaseWhilePressed(const QPointF &, qint64)
{
    if (overshootPosition != QPointF(0.0, 0.0))
        setState(QtScroller::Scrolling);
    else
        setState(QtScroller::Inactive);
    return false;
}

bool QtScrollerPrivate::moveWhilePressed(const QPointF &position, qint64 timestamp)
{
    Q_Q(QtScroller);
    const QtScrollerPropertiesPrivate *sp = properties.d.data();
    QPointF ppm = q->pixelPerMeter();

    QPointF deltaPixel = position - pressPosition;

    bool moveAborted = false;
    bool moveStarted = (((deltaPixel / ppm).manhattanLength()) > sp->dragStartDistance);

    // check the direction of the mouse drag and abort if it's too much in the wrong direction.
    if (moveStarted) {
        QRectF max = contentPosRange;
        bool canScrollX = (max.width() > 0);
        bool canScrollY = (max.height() > 0);

        if (sp->hOvershootPolicy == QtScrollerProperties::OvershootAlwaysOn)
            canScrollX = true;
        if (sp->vOvershootPolicy == QtScrollerProperties::OvershootAlwaysOn)
            canScrollY = true;

        if (qAbs(deltaPixel.x() / ppm.x()) < qAbs(deltaPixel.y() / ppm.y())) {
            if (!canScrollY)
                moveAborted = true;
        } else {
            if (!canScrollX)
                moveAborted = true;
        }
    }

    if (moveAborted) {
        setState(QtScroller::Inactive);
        moveStarted = false;

    } else if (moveStarted) {
        setState(QtScroller::Dragging);

        // subtract the dragStartDistance
        deltaPixel = deltaPixel - deltaPixel * (sp->dragStartDistance / deltaPixel.manhattanLength());

        if (!deltaPixel.isNull()) {
            // handleDrag updates lastPosition, lastTimestamp and velocity
            handleDrag(pressPosition + deltaPixel, timestamp);
        }
    }
    return moveStarted;
}

bool QtScrollerPrivate::moveWhileDragging(const QPointF &position, qint64 timestamp)
{
    // handleDrag updates lastPosition, lastTimestamp and velocity
    handleDrag(position, timestamp);
    return true;
}

void QtScrollerPrivate::timerEventWhileDragging()
{
    if (!dragDistance.isNull()) {
        qScrollerDebug() << "QScroller::timerEventWhileDragging() -- dragDistance:" << dragDistance;

        setContentPositionHelperDragging(-dragDistance);
        dragDistance = QPointF(0, 0);
    }
}

bool QtScrollerPrivate::releaseWhileDragging(const QPointF &position, qint64 timestamp)
{
    Q_Q(QtScroller);
    const QtScrollerPropertiesPrivate *sp = properties.d.data();

    // check if we moved at all - this can happen if you stop a running
    // scroller with a press and release shortly afterwards
    QPointF deltaPixel = position - pressPosition;
    if (((deltaPixel / q->pixelPerMeter()).manhattanLength()) > sp->dragStartDistance) {

        // handle accelerating flicks
        if (!oldVelocity.isNull() && sp->acceleratingFlickMaximumTime &&
            ((timestamp - pressTimestamp) < qint64(sp->acceleratingFlickMaximumTime * 1000))) {

            // - determine if the direction was changed
            int signX = 0, signY = 0;
            if (releaseVelocity.x())
                signX = (releaseVelocity.x() > 0) == (oldVelocity.x() > 0) ? 1 : -1;
            if (releaseVelocity.y())
                signY = (releaseVelocity.y() > 0) == (oldVelocity.y() > 0) ? 1 : -1;

            if (signX > 0)
                releaseVelocity.setX(qBound(-sp->maximumVelocity,
                                            oldVelocity.x() * sp->acceleratingFlickSpeedupFactor,
                                            sp->maximumVelocity));
            if (signY > 0)
                releaseVelocity.setY(qBound(-sp->maximumVelocity,
                                            oldVelocity.y() * sp->acceleratingFlickSpeedupFactor,
                                            sp->maximumVelocity));
        }
    }

    QPointF ppm = q->pixelPerMeter();
    createScrollingSegments(releaseVelocity.x(), contentPosition.x() + overshootPosition.x(), ppm.x(), Qt::Horizontal);
    createScrollingSegments(releaseVelocity.y(), contentPosition.y() + overshootPosition.y(), ppm.y(), Qt::Vertical);

    qScrollerDebug() << "QScroller::releaseWhileDragging() -- velocity:" << releaseVelocity << "-- minimum velocity:" << sp->minimumVelocity << "overshoot" << overshootPosition;

    if (xSegments.isEmpty() && ySegments.isEmpty())
        setState(QtScroller::Inactive);
    else
        setState(QtScroller::Scrolling);

    return true;
}

void QtScrollerPrivate::timerEventWhileScrolling()
{
    qScrollerDebug() << "QScroller::timerEventWhileScrolling()";

    setContentPositionHelperScrolling();
    if (xSegments.isEmpty() && ySegments.isEmpty())
        setState(QtScroller::Inactive);
}

bool QtScrollerPrivate::pressWhileScrolling(const QPointF &position, qint64 timestamp)
{
    lastPosition = pressPosition = position;
    lastTimestamp = pressTimestamp = timestamp;
    setState(QtScroller::Pressed);
    setState(QtScroller::Dragging);
    return true;
}

/*! \internal
    This function handles all state changes of the scroller.
*/
void QtScrollerPrivate::setState(QtScroller::State newstate)
{
    Q_Q(QtScroller);
    bool sendLastScroll = false;

    if (state == newstate)
        return;

    qScrollerDebug() << q << "QScroller::setState(" << stateName(newstate) << ")";

    switch (newstate) {
    case QtScroller::Inactive:
        scrollTimer->stop();

        // send the last scroll event (but only after the current state change was finished)
        if (!firstScroll)
            sendLastScroll = true;

        releaseVelocity = QPointF(0, 0);
        break;

    case QtScroller::Pressed:
        scrollTimer->stop();

        oldVelocity = releaseVelocity;
        releaseVelocity = QPointF(0, 0);
        break;

    case QtScroller::Dragging:
        dragDistance = QPointF(0, 0);
        if (state == QtScroller::Pressed)
            scrollTimer->start();
        break;

    case QtScroller::Scrolling:
        scrollTimer->start();
        break;
    }

    qSwap(state, newstate);

    if (sendLastScroll) {
        QtScrollEvent se(contentPosition, overshootPosition, QtScrollEvent::ScrollFinished);
        sendEvent(target, &se);
        firstScroll = true;
    }
    if (state == QtScroller::Dragging || state == QtScroller::Scrolling)
        activeScrollers.insert(q);
    else
        activeScrollers.remove(q);
    emit q->stateChanged(state);
}


/*! \internal
    Helps when setting the content position.
    It will try to move the content by the requested delta but stop in case
    when we are coming back from an overshoot or a scrollTo.
    It will also indicate a new overshooting condition by the overshootX and oversthootY flags.

    In this cases it will reset the velocity variables and other flags.

    Also keeps track of the current over-shooting value in overshootPosition.

    \a deltaPos is the amount of pixels the current content position should be moved
*/
void QtScrollerPrivate::setContentPositionHelperDragging(const QPointF &deltaPos)
{
    Q_Q(QtScroller);
    QPointF ppm = q->pixelPerMeter();
    const QtScrollerPropertiesPrivate *sp = properties.d.data();
    QPointF v = q->velocity();

    if (sp->overshootDragResistanceFactor)
        overshootPosition /= sp->overshootDragResistanceFactor;

    QPointF oldPos = contentPosition + overshootPosition;
    QPointF newPos = oldPos + deltaPos;

    qScrollerDebug() << "QScroller::setContentPositionHelperDragging(" << deltaPos << " [pix])";
    qScrollerDebug() << "  --> overshoot:" << overshootPosition << "- old pos:" << oldPos << "- new pos:" << newPos;

    QPointF oldClampedPos = clampToRect(oldPos, contentPosRange);
    QPointF newClampedPos = clampToRect(newPos, contentPosRange);

    // --- handle overshooting and stop if the coordinate is going back inside the normal area
    bool alwaysOvershootX = (sp->hOvershootPolicy == QtScrollerProperties::OvershootAlwaysOn);
    bool alwaysOvershootY = (sp->vOvershootPolicy == QtScrollerProperties::OvershootAlwaysOn);
    bool noOvershootX = (sp->hOvershootPolicy == QtScrollerProperties::OvershootAlwaysOff) ||
                        ((state == QtScroller::Dragging) && !sp->overshootDragResistanceFactor) ||
                        !sp->overshootMaximumDistanceFactor;
    bool noOvershootY = (sp->vOvershootPolicy == QtScrollerProperties::OvershootAlwaysOff) ||
                        ((state == QtScroller::Dragging) && !sp->overshootDragResistanceFactor) ||
                        !sp->overshootMaximumDistanceFactor;
    bool canOvershootX = !noOvershootX && (alwaysOvershootX || contentPosRange.width());
    bool canOvershootY = !noOvershootY && (alwaysOvershootY || contentPosRange.height());

    qreal oldOvershootX = (canOvershootX) ? oldPos.x() - oldClampedPos.x() : 0;
    qreal oldOvershootY = (canOvershootY) ? oldPos.y() - oldClampedPos.y() : 0;

    qreal newOvershootX = (canOvershootX) ? newPos.x() - newClampedPos.x() : 0;
    qreal newOvershootY = (canOvershootY) ? newPos.y() - newClampedPos.y() : 0;

    qreal maxOvershootX = viewportSize.width() * sp->overshootMaximumDistanceFactor;
    qreal maxOvershootY = viewportSize.height() * sp->overshootMaximumDistanceFactor;

    if (sp->overshootDragResistanceFactor) {
        oldOvershootX *= sp->overshootDragResistanceFactor;
        oldOvershootY *= sp->overshootDragResistanceFactor;
        newOvershootX *= sp->overshootDragResistanceFactor;
        newOvershootY *= sp->overshootDragResistanceFactor;
    }

    // -- stop at the maximum overshoot distance

    newOvershootX = qBound(-maxOvershootX, newOvershootX, maxOvershootX);
    newOvershootY = qBound(-maxOvershootY, newOvershootY, maxOvershootY);

    overshootPosition.setX(newOvershootX);
    overshootPosition.setY(newOvershootY);
    contentPosition = newClampedPos;

    QtScrollEvent se(contentPosition, overshootPosition, firstScroll ? QtScrollEvent::ScrollStarted : QtScrollEvent::ScrollUpdated);
    sendEvent(target, &se);
    firstScroll = false;

    qScrollerDebug() << "  --> new position:" << newClampedPos << "- new overshoot:" << overshootPosition <<
                        "- overshoot x/y?:" << overshootPosition;
}


qreal QtScrollerPrivate::nextSegmentPosition(QQueue<ScrollSegment> &segments, qint64 now, qreal oldPos)
{
    qreal pos = oldPos;

    // check the X segments for new positions
    while (!segments.isEmpty()) {
        const ScrollSegment s = segments.head();
        bool finished = false;
        qreal progress = 0;

        if ((s.startTime + s.deltaTime) <= now) {
            progress = s.maxProgress;
            finished = true;
        } else if (s.startTime <= now) {
            progress = qreal(now - s.startTime) / (qreal(s.deltaTime) / s.maxProgress);
        } else {
            break;
        }

        pos = s.startPos + s.deltaPos * s.curve.valueForProgress(progress) / s.curve.valueForProgress(s.maxProgress);
        if (finished)
            segments.dequeue();
        else
            break;
    }
    return pos;
}

void QtScrollerPrivate::setContentPositionHelperScrolling()
{
    qint64 now = monotonicTimer.elapsed();
    QPointF newPos = contentPosition + overshootPosition;

    newPos.setX(nextSegmentPosition(xSegments, now, newPos.x()));
    newPos.setY(nextSegmentPosition(ySegments, now, newPos.y()));

    // -- set the position and handle overshoot
    qScrollerDebug() << "QScroller::setContentPositionHelperScrolling()";
    qScrollerDebug() << "  --> overshoot:" << overshootPosition << "- new pos:" << newPos;

    QPointF newClampedPos = clampToRect(newPos, contentPosRange);

    overshootPosition.setX(newPos.x() - newClampedPos.x());
    overshootPosition.setY(newPos.y() - newClampedPos.y());
    contentPosition = newClampedPos;

    QtScrollEvent se(contentPosition, overshootPosition, firstScroll ? QtScrollEvent::ScrollStarted : QtScrollEvent::ScrollUpdated);
    sendEvent(target, &se);
    firstScroll = false;

    qScrollerDebug() << "  --> new position:" << newClampedPos << "- new overshoot:" << overshootPosition;
}

/*! \internal
 *  Returns the next snap point in direction.
 *  If \a direction >0 it will return the next snap point that is larger than the current position.
 *  If \a direction <0 it will return the next snap point that is smaller than the current position.
 *  If \a direction ==0 it will return the nearest snap point (or the current position if we are already
 *  on a snap point.
 *  Returns the nearest snap position or NaN if no such point could be found.
 */
qreal QtScrollerPrivate::nextSnapPos(qreal p, int dir, Qt::Orientation orientation)
{
    qreal bestSnapPos = NAN;
    qreal bestSnapPosDist = INFINITY;

    qreal minPos;
    qreal maxPos;

    if (orientation == Qt::Horizontal) {
        minPos = contentPosRange.left();
        maxPos = contentPosRange.right();
    } else {
        minPos = contentPosRange.top();
        maxPos = contentPosRange.bottom();
    }

    if (orientation == Qt::Horizontal) {
        // the snap points in the list
        foreach (qreal snapPos, snapPositionsX) {
            qreal snapPosDist = snapPos - p;
            if ((dir > 0 && snapPosDist < 0) ||
                (dir < 0 && snapPosDist > 0))
                continue; // wrong direction
            if (snapPos < minPos || snapPos > maxPos)
                continue; // invalid

            if (qIsNaN(bestSnapPos) ||
                qAbs(snapPosDist) < bestSnapPosDist) {
                bestSnapPos = snapPos;
                bestSnapPosDist = qAbs(snapPosDist);
            }
        }

        // the snap point interval
        if (snapIntervalX > 0.0) {
            // this is the nearest point (because we used qRound)
            qreal snapPos = qRound((p - snapFirstX) / snapIntervalX) * snapIntervalX + snapFirstX;
            if (dir > 0 && snapPos < p)
                snapPos += snapIntervalX;
            else if (dir < 0 && snapPos > p)
                snapPos -= snapIntervalX;

            if (snapPos >= minPos && snapPos <= maxPos) {
                qreal snapPosDist = snapPos - p;

                if (qIsNaN(bestSnapPos) ||
                    qAbs(snapPosDist) < bestSnapPosDist) {
                    bestSnapPos = snapPos;
                    bestSnapPosDist = qAbs(snapPosDist);
                }
            }
        }

    } else { // (orientation == Qt::Vertical)
        // the snap points in the list
        foreach (qreal snapPos, snapPositionsY) {
            qreal snapPosDist = snapPos - p;
            if ((dir > 0 && snapPosDist < 0) ||
                (dir < 0 && snapPosDist > 0))
                continue; // wrong direction
            if (snapPos < minPos || snapPos > maxPos)
                continue; // invalid

            if (qIsNaN(bestSnapPos) ||
                qAbs(snapPosDist) < bestSnapPosDist) {
                bestSnapPos = snapPos;
                bestSnapPosDist = qAbs(snapPosDist);
            }
        }

        // the snap point interval
        if (snapIntervalY > 0.0) {
            // this is the nearest point (because we used qRound)
            qreal snapPos = qRound((p - snapFirstY) / snapIntervalY) * snapIntervalY + snapFirstY;
            if (dir > 0 && snapPos < p)
                snapPos += snapIntervalY;
            else if (dir < 0 && snapPos > p)
                snapPos -= snapIntervalY;

            if (snapPos >= minPos && snapPos <= maxPos) {
                qreal snapPosDist = snapPos - p;

                if (qIsNaN(bestSnapPos) ||
                    qAbs(snapPosDist) < bestSnapPosDist) {
                    bestSnapPos = snapPos;
                    bestSnapPosDist = qAbs(snapPosDist);
                }
            }
        }
    }

    return bestSnapPos;
}

/*!
    \enum QScroller::State

    This enum contains the different QScroller states.

    \value Inactive The scroller is not scrolling and nothing is pressed.
    \value Pressed A touch event was received or the mouse button pressed but the scroll area is currently not dragged.
    \value Dragging The scroll area is currently following the touch point or mouse.
    \value Scrolling The scroll area is moving on it's own.
*/

/*!
    \enum QScroller::ScrollerGestureType

    This enum contains the different gesture types that are supported by the QScroller gesture recognizer.

    \value TouchGesture The gesture recognizer will only trigger on touch
    events.  Specifically it will react on single touch points when using a
    touch screen and dual touch points when using a touchpad.
    \value LeftMouseButtonGesture The gesture recognizer will only trigger on left mouse button events.
    \value MiddleMouseButtonGesture The gesture recognizer will only trigger on middle mouse button events.
    \value RightMouseButtonGesture The gesture recognizer will only trigger on right mouse button events.
*/

