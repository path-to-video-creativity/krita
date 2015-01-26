/*
 *  Copyright (c) 2000 Matthias Elter <elter@kde.org>
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef KISGLOBAL_H_
#define KISGLOBAL_H_

#include <limits.h>

#include <calligraversion.h>

#include <KoConfig.h>
#include "kis_assert.h"

#include <QPoint>
#include <QPointF>

#define KRITA_VERSION CALLIGRA_VERSION

const quint8 quint8_MAX = UCHAR_MAX;
const quint16 quint16_MAX = 65535;

const qint32 qint32_MAX = (2147483647);
const qint32 qint32_MIN = (-2147483647 - 1);

const quint8 MAX_SELECTED = UCHAR_MAX;
const quint8 MIN_SELECTED = 0;
const quint8 SELECTION_THRESHOLD = 1;

enum enumCursorStyle {
    CURSOR_STYLE_TOOLICON = 0,
    CURSOR_STYLE_CROSSHAIR = 1,
    CURSOR_STYLE_POINTER = 2,
    CURSOR_STYLE_OUTLINE = 3,
    CURSOR_STYLE_NO_CURSOR = 4,
    CURSOR_STYLE_SMALL_ROUND = 5,
    CURSOR_STYLE_OUTLINE_CENTER_DOT = 6,
    CURSOR_STYLE_OUTLINE_CENTER_CROSS = 7,
    CURSOR_STYLE_TRIANGLE_RIGHTHANDED = 8,
    CURSOR_STYLE_TRIANGLE_LEFTHANDED = 9,
    CURSOR_STYLE_OUTLINE_TRIANGLE_RIGHTHANDED = 10,
    CURSOR_STYLE_OUTLINE_TRIANGLE_LEFTHANDED = 11
};

/*
 * Most wacom pads have 512 levels of pressure; Qt only supports 256, and even
 * this is downscaled to 127 levels because the line would be too jittery, and
 * the amount of masks take too much memory otherwise.
 */
const qint32 PRESSURE_LEVELS = 127;
const double PRESSURE_MIN = 0.0;
const double PRESSURE_MAX = 1.0;
const double PRESSURE_DEFAULT = PRESSURE_MAX;
const double PRESSURE_THRESHOLD = 5.0 / 255.0;

// copy of lcms.h
#define INTENT_PERCEPTUAL                 0
#define INTENT_RELATIVE_COLORIMETRIC      1
#define INTENT_SATURATION                 2
#define INTENT_ABSOLUTE_COLORIMETRIC      3

#include <cmath>
#include <QPointF>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// converts \p a to [0, 2 * M_PI) range
inline qreal normalizeAngle(qreal a) {
    if (a < 0.0) {
        a = 2 * M_PI + fmod(a, 2 * M_PI);
    }

    return a > 2 * M_PI ? fmod(a, 2 * M_PI) : a;
}

// converts \p a to [0, 360.0) range
inline qreal normalizeAngleDegrees(qreal a) {
    if (a < 0.0) {
        a = 360.0 + fmod(a, 360.0);
    }

    return a > 360.0 ? fmod(a, 360.0) : a;
}

inline qreal shortestAngularDistance(qreal a, qreal b) {
    qreal dist = fmod(qAbs(a - b), 2 * M_PI);
    if (dist > M_PI) dist = 2 * M_PI - dist;

    return dist;
}

inline qreal incrementInDirection(qreal a, qreal inc, qreal direction) {
    qreal b1 = a + inc;
    qreal b2 = a - inc;

    qreal d1 = shortestAngularDistance(b1, direction);
    qreal d2 = shortestAngularDistance(b2, direction);

    return d1 < d2 ? b1 : b2;
}

template<typename T>
inline T pow2(const T& x) {
    return x * x;
}

template<typename T>
inline T kisDegreesToRadians(T degrees) {
    return degrees * M_PI / 180.0;
}

template<typename T>
inline T kisRadiansToDegrees(T radians) {
    return radians * 180.0 / M_PI;
}

template<class T, typename U>
inline T kisGrowRect(const T &rect, U offset) {
    return rect.adjusted(-offset, -offset, offset, offset);
}

inline qreal kisDistance(const QPointF &pt1, const QPointF &pt2) {
    return std::sqrt(pow2(pt1.x() - pt2.x()) + pow2(pt1.y() - pt2.y()));
}

inline qreal kisSquareDistance(const QPointF &pt1, const QPointF &pt2) {
    return pow2(pt1.x() - pt2.x()) + pow2(pt1.y() - pt2.y());
}

#include <QLineF>

inline qreal kisDistanceToLine(const QPointF &m, const QLineF &line)
{
    const QPointF &p1 = line.p1();
    const QPointF &p2 = line.p2();

    qreal distance = 0;

    if (qFuzzyCompare(p1.x(), p2.x())) {
        distance = qAbs(m.x() - p2.x());
    } else if (qFuzzyCompare(p1.y(), p2.y())) {
        distance = qAbs(m.y() - p2.y());
    } else {
        qreal A = 1;
        qreal B = - (p1.x() - p2.x()) / (p1.y() - p2.y());
        qreal C = - p1.x() - B * p1.y();

        distance = qAbs(A * m.x() + B * m.y() + C) / std::sqrt(pow2(A) + pow2(B));
    }

    return distance;
}

inline QPointF kisProjectOnVector(const QPointF &base, const QPointF &v)
{
    const qreal prod = base.x() * v.x() + base.y() * v.y();
    const qreal lengthSq = pow2(base.x()) + pow2(base.y());
    qreal coeff = prod / lengthSq;

    return coeff * base;
}

#include <QRect>

inline QRect kisEnsureInRect(QRect rc, const QRect &bounds)
{
    if(rc.right() > bounds.right()) {
        rc.translate(bounds.right() - rc.right(), 0);
    }

    if(rc.left() < bounds.left()) {
        rc.translate(bounds.left() - rc.left(), 0);
    }

    if(rc.bottom() > bounds.bottom()) {
        rc.translate(0, bounds.bottom() - rc.bottom());
    }

    if(rc.top() < bounds.top()) {
        rc.translate(0, bounds.top() - rc.top());
    }

    return rc;
}

#include <QSharedPointer>

template <class T>
inline QSharedPointer<T> toQShared(T* ptr) {
    return QSharedPointer<T>(ptr);
}


#endif // KISGLOBAL_H_

