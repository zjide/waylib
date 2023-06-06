// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wquickseat_p.h"
#include "wseat.h"
#include "wquickcursor.h"
#include "woutput.h"

#include <qwcursor.h>
#include <qwoutput.h>

#include <QRect>

extern "C" {
#include <wlr/types/wlr_cursor.h>
}

QW_USE_NAMESPACE
WAYLIB_SERVER_BEGIN_NAMESPACE

class WQuickSeatPrivate : public WObjectPrivate
{
public:
    WQuickSeatPrivate(WQuickSeat *qq)
        : WObjectPrivate(qq)
    {

    }

    inline QWCursor *qwcursor() const {
        return cursor->nativeInterface<QWCursor>();
    }
    inline void resetCursorRegion() {
        // TODO fix qwlroots: using nullptr to wlr_cursor_map_to_region if the QRect is Null
        wlr_cursor_map_to_region(qwcursor()->handle(), nullptr);
    }

    void updateCursorMap();

    W_DECLARE_PUBLIC(WQuickSeat)

    WSeat *seat = nullptr;
    QString name;
    QList<WOutput*> outputs;
    WQuickCursor *cursor = nullptr;
};

void WQuickSeatPrivate::updateCursorMap()
{
    auto cursor = qwcursor();
    Q_ASSERT(cursor);

    if (outputs.size() > 1) {
        QPoint minPos(INT_MAX, INT_MAX);
        QPoint maxPos(0, 0);

        for (auto o : outputs) {
            QRect rect = o->layout()->getBox(o->nativeInterface<QWOutput>()->handle());

            if (rect.x() < minPos.x())
                minPos.rx() = rect.x();
            if (rect.y() < minPos.y())
                minPos.ry() = rect.y();

            if (rect.right() > maxPos.x())
                maxPos.rx() = rect.right();
            if (rect.bottom() < maxPos.y())
                maxPos.ry() = rect.bottom();
        }

        cursor->mapToRegion(QRect(minPos, maxPos));
        cursor->mapToOutput(nullptr);
    } else if (outputs.size() == 1) {
        cursor->mapToOutput(outputs.first()->nativeInterface<QWOutput>()->handle());
        resetCursorRegion();
    } else {
        cursor->mapToOutput(nullptr);
        resetCursorRegion();
    }
}

WQuickSeat::WQuickSeat(QObject *parent)
    : WQuickWaylandServerInterface(parent)
    , WObject(*new WQuickSeatPrivate(this))
{

}

QString WQuickSeat::name() const
{
    W_DC(WQuickSeat);

    return d->name;
}

void WQuickSeat::setName(const QString &newName)
{
    W_D(WQuickSeat);

    d->name = newName;
}

WQuickCursor *WQuickSeat::cursor() const
{
    W_DC(WQuickSeat);
    return d->cursor;
}

void WQuickSeat::setCursor(WQuickCursor *cursor)
{
    W_D(WQuickSeat);
    if (d->cursor == cursor)
        return;

    d->cursor = cursor;

    if (d->seat) {
        d->seat->setCursor(cursor);
        d->updateCursorMap();
    }
    Q_EMIT cursorChanged();
}

WSeat *WQuickSeat::seat() const
{
    W_DC(WQuickSeat);
    return d->seat;
}

void WQuickSeat::addDevice(WInputDevice *device)
{
    W_D(WQuickSeat);

    d->seat->attachInputDevice(device);
}

void WQuickSeat::addOutput(WOutput *output)
{
    W_D(WQuickSeat);
    Q_ASSERT(!d->outputs.contains(output));
    d->outputs.append(output);

    if (d->seat && d->seat->cursor())
        d->updateCursorMap();
}

void WQuickSeat::removeOutput(WOutput *output)
{
    W_D(WQuickSeat);
    Q_ASSERT(d->outputs.contains(output));
    d->outputs.removeOne(output);

    if (d->seat && d->seat->cursor())
        d->updateCursorMap();
}

void WQuickSeat::create()
{
    WQuickWaylandServerInterface::create();

    W_D(WQuickSeat);
    Q_ASSERT(!d->name.isEmpty());
    d->seat = server()->attach<WSeat>(d->name.toUtf8());
    Q_EMIT seatChanged();
}

void WQuickSeat::polish()
{
    W_D(WQuickSeat);
    if (d->cursor) {
        d->seat->setCursor(d->cursor);
        d->updateCursorMap();
    }
}

WAYLIB_SERVER_END_NAMESPACE
