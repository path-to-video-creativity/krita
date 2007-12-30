/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Clarence Dang <dang@kde.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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

#include <QRect>

#include <kis_debug.h>

#include "kis_brush.h"
#include "kis_global.h"
#include "kis_paint_device.h"

#include "kis_painter.h"
#include "kis_types.h"
#include "kis_paintop.h"
#include "kis_iterators_pixel.h"
#include "KoColorSpace.h"
#include "kis_selection.h"
#include "kis_eraseop.h"

KisPaintOp * KisEraseOpFactory::createOp(const KisPaintOpSettings */*settings*/, KisPainter * painter, KisImageSP image)
{
    Q_UNUSED( image )
    KisPaintOp * op = new KisEraseOp(painter);
    Q_CHECK_PTR(op);
    return op;
}


KisEraseOp::KisEraseOp(KisPainter * painter)
    : KisPaintOp(painter)
{
}

KisEraseOp::~KisEraseOp()
{
}

void KisEraseOp::paintAt(const KisPaintInformation& info)
{
// Erasing is traditionally in paint applications one of two things:
// either it is painting in the 'background' color, or it is replacing
// all pixels with transparent (black?) pixels.
//
// That's what this paint op does for now; however, anyone who has
// ever worked with paper and soft pencils knows that a sharp piece of
// eraser rubber is a pretty useful too for making sharp to fuzzy lines
// in the graphite layer, or equally useful: for smudging skin tones.
//
// A smudge tool for Krita is in the making, but when working with
// a tablet, the eraser tip should be at least as functional as a rubber eraser.
// That means that only after repeated or forceful application should all the
// 'paint' or 'graphite' be removed from the surface -- a kind of pressure
// sensitive, incremental smudge.
//
// And there should be an option to not have the eraser work on certain
// kinds of material. Layers are just a hack for this; putting your ink work
// in one layer and your pencil in another is not the same as really working
// with the combination.

    if (!painter()) return;

    KisPaintDeviceSP device = painter()->device();
    if (!device) return;

    KisBrush *brush = painter()->brush();
    if (! brush->canPaintFor(info) )
        return;
    double scale = KisPaintOp::scaleForPressure( info.pressure() );
    QPointF hotSpot = brush->hotSpot(scale, scale);
    QPointF pt = info.pos() - hotSpot;

    qint32 destX;
    double xFraction;
    qint32 destY;
    double yFraction;

    splitCoordinate(pt.x(), &destX, &xFraction);
    splitCoordinate(pt.y(), &destY, &yFraction);

    KisPaintDeviceSP dab = cachedDab( );
    brush->mask(dab, scale, scale, 0.0, info, xFraction, yFraction);

    KisRectIteratorPixel it = dab->createRectIterator(0, 0, brush->maskWidth(scale, 0.0), brush->maskHeight(scale, 0.0));
    const KoColorSpace* cs = dab->colorSpace();
    while (!it.isDone()) {
        cs->setAlpha(it.rawData(), quint8_MAX - cs->alpha(it.rawData()), 1);
        ++it;
    }

    QRect dabRect = QRect(0, 0, brush->maskWidth(scale, 0.0),
                          brush->maskHeight(scale, 0.0));
    QRect dstRect = QRect(destX, destY, dabRect.width(), dabRect.height());


    if ( painter()->bounds().isValid() ) {
        dstRect &= painter()->bounds();
    }

    if (dstRect.isNull() || dstRect.isEmpty() || !dstRect.isValid()) return;

    qint32 sx = dstRect.x() - destX;
    qint32 sy = dstRect.y() - destY;
    qint32 sw = dstRect.width();
    qint32 sh = dstRect.height();

    painter()->bltSelection(dstRect.x(), dstRect.y(), COMPOSITE_ERASE, dab, painter()->opacity(), sx, sy, sw, sh);
}

