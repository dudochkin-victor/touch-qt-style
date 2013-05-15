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

#ifndef QTSCROLLERPROPERTIES_H
#define QTSCROLLERPROPERTIES_H

#include <QtCore/QScopedPointer>
#include <QtCore/QMetaType>
#include <QtCore/QVariant>

#if (QT_VERSION < QT_VERSION_CHECK(4, 7, 0))
#  include <QEasingCurve>
Q_DECLARE_METATYPE(QEasingCurve)
#endif

class QtScroller;
class QtScrollerPrivate;
class QtScrollerPropertiesPrivate;

class QtScrollerProperties
{
public:
    QtScrollerProperties();
    QtScrollerProperties(const QtScrollerProperties &sp);
    QtScrollerProperties &operator=(const QtScrollerProperties &sp);
    virtual ~QtScrollerProperties();

    bool operator==(const QtScrollerProperties &sp) const;
    bool operator!=(const QtScrollerProperties &sp) const;

    static void setDefaultScrollerProperties(const QtScrollerProperties &sp);
    static void unsetDefaultScrollerProperties();

    enum ScrollMetric
    {
        DragVelocitySmoothingFactor,             // qreal [0..1/s]  (complex calculation involving time) v = v_new* DASF + v_old * (1-DASF)

        OvershootDragResistanceFactor,           // qreal [0..1]

        SnapPositionRatio,                       // qreal [0..1]
        SnapTime,                                // qreal [s]

        MousePressEventDelay,                    // qreal [s]
        DragStartDistance,                       // qreal [m]

        MinimumVelocity,                         // qreal [m/s]
        MaximumVelocity,                         // qreal [m/s]

        MaximumClickThroughVelocity,             // qreal [m/s]
        AxisLockThreshold,                       // qreal [0..1] atan(|min(dx,dy)|/|max(dx,dy)|)

        AcceleratingFlickMaximumTime,            // qreal [s]
        AcceleratingFlickSpeedupFactor,          // qreal [1..]

        ScrollingCurve,                          // QEasingCurve
        DecelerationFactor,                      // slope of the curve

        OvershootTime,                           // qreal [s]
        OvershootCurve,                          // QEasingCurve
        OvershootMaximumDistanceFactor,          // slope of the curve

        ScrollMetricCount
    };

    QVariant scrollMetric(ScrollMetric metric) const;
    void setScrollMetric(ScrollMetric metric, const QVariant &value);

    enum OvershootPolicy
    {
        OvershootWhenScrollable,
        OvershootAlwaysOff,
        OvershootAlwaysOn,
    };

    OvershootPolicy horizontalOvershootPolicy() const;
    void setHorizontalOvershootPolicy(OvershootPolicy policy);
    OvershootPolicy verticalOvershootPolicy() const;
    void setVerticalOvershootPolicy(OvershootPolicy policy);

protected:
    QScopedPointer<QtScrollerPropertiesPrivate> d;

private:
    QtScrollerProperties(QtScrollerPropertiesPrivate &dd);

    friend class QtScrollerPropertiesPrivate;
    friend class QtScroller;
    friend class QtScrollerPrivate;
};

Q_DECLARE_METATYPE(QtScrollerProperties *)

#endif // QTSCROLLERPROPERTIES_H
