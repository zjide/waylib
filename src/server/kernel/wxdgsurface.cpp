// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wxdgsurface.h"
#include "private/wsurface_p.h"
#include "wseat.h"

#include <qwxdgshell.h>
#include <qwseat.h>

#include <QDebug>

extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/edges.h>
}

QW_USE_NAMESPACE
WAYLIB_SERVER_BEGIN_NAMESPACE

class Q_DECL_HIDDEN WXdgSurfacePrivate : public WSurfacePrivate {
public:
    WXdgSurfacePrivate(WXdgSurface *qq, void *handle, WServer *server);

    // begin slot function
    void on_configure(wlr_xdg_surface_configure *event);
    void on_ack_configure(wlr_xdg_surface_configure *event);
    void on_map();
    void on_unmap();

    // toplevel
    void on_request_move(wlr_xdg_toplevel_move_event *event);
    void on_request_resize(wlr_xdg_toplevel_resize_event *event);
    void on_request_maximize(bool maximize);
    // end slot function

    void init();
    void connect();

    W_DECLARE_PUBLIC(WXdgSurface)

    QWXdgSurface *handle;
};

WXdgSurfacePrivate::WXdgSurfacePrivate(WXdgSurface *qq, void *hh, WServer *server)
    : WSurfacePrivate(qq, server)
    , handle(reinterpret_cast<QWXdgSurface*>(hh))
{
}

void WXdgSurfacePrivate::on_configure(wlr_xdg_surface_configure *event)
{
    Q_UNUSED(event)
//    auto config = reinterpret_cast<wlr_xdg_surface_configure*>(data);
}

void WXdgSurfacePrivate::on_ack_configure(wlr_xdg_surface_configure *event)
{
    Q_UNUSED(event)
//    auto config = reinterpret_cast<wlr_xdg_surface_configure*>(data);
}

void WXdgSurfacePrivate::on_map()
{
    Q_EMIT q_func()->requestMap();
}

void WXdgSurfacePrivate::on_unmap()
{
    Q_EMIT q_func()->requestUnmap();
}

void WXdgSurfacePrivate::on_request_move(wlr_xdg_toplevel_move_event *event)
{
    auto surface = WXdgSurface::fromHandle<QWXdgSurface>(QWXdgToplevel::from(event->toplevel));
    Q_ASSERT(surface == q_func());
    Q_EMIT q_func()->requestMove(WSeat::fromHandle<QWSeat>(QWSeat::from(event->seat->seat)), event->serial);
}

inline static Qt::Edges toQtEdge(uint32_t edges) {
    Qt::Edges qedges = Qt::Edges();

    if (edges & WLR_EDGE_TOP) {
        qedges |= Qt::TopEdge;
    }

    if (edges & WLR_EDGE_BOTTOM) {
        qedges |= Qt::BottomEdge;
    }

    if (edges & WLR_EDGE_LEFT) {
        qedges |= Qt::LeftEdge;
    }

    if (edges & WLR_EDGE_RIGHT) {
        qedges |= Qt::RightEdge;
    }

    return qedges;
}

void WXdgSurfacePrivate::on_request_resize(wlr_xdg_toplevel_resize_event *event)
{
    auto seat = WSeat::fromHandle<QWSeat>(QWSeat::from(event->seat->seat));
    auto surface = WXdgSurface::fromHandle<QWXdgSurface>(QWXdgToplevel::from(event->toplevel));
    Q_ASSERT(surface == q_func());
    Q_EMIT q_func()->requestResize(seat, toQtEdge(event->edges), event->serial);
}

void WXdgSurfacePrivate::on_request_maximize(bool maximize)
{
    if (maximize) {
        q_func()->requestMaximize();
    } else {
        q_func()->requestUnmaximize();
    }
}

void WXdgSurfacePrivate::init()
{
    W_Q(WXdgSurface);
    handle->handle()->data = q;
    q->setHandle(reinterpret_cast<WSurfaceHandle*>(handle->handle()->surface));

    connect();
}

void WXdgSurfacePrivate::connect()
{
    QObject::connect(handle, &QWXdgSurface::configure, q_func(), [this] (wlr_xdg_surface_configure *event) {
        on_configure(event);
    });
    QObject::connect(handle, &QWXdgSurface::ackConfigure, q_func(), [this] (wlr_xdg_surface_configure *event) {
        on_ack_configure(event);
    });

    QObject::connect(handle, &QWXdgSurface::map, q_func(), [this] {
        on_map();
    });
    QObject::connect(handle, &QWXdgSurface::unmap, q_func(), [this] {
        on_unmap();
    });

    if (auto toplevel = handle->topToplevel()) {
        QObject::connect(toplevel, &QWXdgToplevel::requestMove, q_func(), [this] (wlr_xdg_toplevel_move_event *event) {
            on_request_move(event);
        });
        QObject::connect(toplevel, &QWXdgToplevel::requestResize, q_func(), [this] (wlr_xdg_toplevel_resize_event *event) {
            on_request_resize(event);
        });
        QObject::connect(toplevel, &QWXdgToplevel::requestMaximize, q_func(), [this] (bool maximize) {
            on_request_maximize(maximize);
        });
    }
}

WXdgSurface::WXdgSurface(WXdgSurfaceHandle *handle, WServer *server, QObject *parent)
    : WSurface(*new WXdgSurfacePrivate(this, handle, server), parent)
{
    d_func()->init();
}

WXdgSurface::~WXdgSurface()
{

}

WSurface::Type *WXdgSurface::toplevelType()
{
    static Type type;
    return &type;
}

WSurface::Type *WXdgSurface::popupType()
{
    static Type type;
    return &type;
}

WSurface::Type *WXdgSurface::noneType()
{
    return nullptr;
}

WSurface::Type *WXdgSurface::type() const
{
    W_DC(WXdgSurface);
    if (d->handle->handle()->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL)
        return toplevelType();
    else if (d->handle->handle()->role == WLR_XDG_SURFACE_ROLE_POPUP)
        return popupType();

    return noneType();
}

bool WXdgSurface::testAttribute(WSurface::Attribute attr) const
{
    W_DC(WXdgSurface);

    if (attr == Attribute::Immovable) {
        return d->handle->handle()->role == WLR_XDG_SURFACE_ROLE_POPUP;
    } else if (attr == Attribute::DoesNotAcceptFocus) {
        return d->handle->handle()->role == WLR_XDG_SURFACE_ROLE_NONE;
    }

    return WSurface::testAttribute(attr);
}

WXdgSurfaceHandle *WXdgSurface::handle() const
{
    W_DC(WXdgSurface);
    return reinterpret_cast<WXdgSurfaceHandle*>(d->handle);
}

WSurfaceHandle *WXdgSurface::inputTargetAt(QPointF &localPos) const
{
    W_DC(WXdgSurface);
    // find a wlr_suface object who can receive the events
    const QPointF pos = localPos;
    auto sur = d->handle->surfaceAt(pos, &localPos);

    return reinterpret_cast<WSurfaceHandle*>(sur);
}

WXdgSurface *WXdgSurface::fromHandle(WXdgSurfaceHandle *handle)
{
    auto *data = reinterpret_cast<QWXdgSurface*>(handle)->handle()->data;
    return reinterpret_cast<WXdgSurface*>(data);
}

bool WXdgSurface::inputRegionContains(const QPointF &localPos) const
{
    W_DC(WXdgSurface);
    return d->handle->surfaceAt(localPos, nullptr);
}

void WXdgSurface::resize(const QSize &size)
{
    W_D(WXdgSurface);

    if (auto toplevel = d->handle->topToplevel()) {
        toplevel->setSize(size);
    }
}

bool WXdgSurface::resizeing() const
{
    W_DC(WXdgSurface);
    return d->handle->handle()->toplevel->current.resizing;
}

QPointF WXdgSurface::position() const
{
    W_DC(WXdgSurface);
    if (auto popup = d->handle->toPopup()) {
        return popup->getPosition();
    }

    return WSurface::position();
}

WSurface *WXdgSurface::parentSurface() const
{
    W_DC(WXdgSurface);

    if (auto toplevel = d->handle->topToplevel()) {
        auto parent = toplevel->handle()->parent;
        if (!parent)
            return nullptr;
        return fromHandle<QWXdgSurface>(QWXdgToplevel::from(parent));
    } else if (auto popup = d->handle->toPopup()) {
        auto parent = popup->handle()->parent;
        if (!parent)
            return nullptr;
        return fromHandle<QWXdgSurface>(QWXdgSurface::from(parent));
    }

    return nullptr;
}

void WXdgSurface::setResizeing(bool resizeing)
{
    W_D(WXdgSurface);
    if (auto toplevel = d->handle->topToplevel()) {
        toplevel->setResizing(resizeing);
    }
}

void WXdgSurface::setMaximize(bool on)
{
    W_D(WXdgSurface);
    if (auto toplevel = d->handle->topToplevel()) {
        toplevel->setMaximized(on);
    }
}

void WXdgSurface::setActivate(bool on)
{
    W_D(WXdgSurface);
    if (auto toplevel = d->handle->topToplevel()) {
        toplevel->setActivated(on);
    }
}

void WXdgSurface::notifyChanged(ChangeType type, std::any oldValue, std::any newValue)
{
    WSurface::notifyChanged(type, oldValue, newValue);
}

void WXdgSurface::notifyBeginState(State state)
{
    if (state == State::Resize) {
        setResizeing(true);
    } else if (state == State::Maximize) {
        setMaximize(true);
    } else if (state == State::Activate) {
        setActivate(true);
    }

    WSurface::notifyBeginState(state);
}

void WXdgSurface::notifyEndState(State state)
{
    if (state == State::Resize) {
        setResizeing(false);
    } else if (state == State::Maximize) {
        setMaximize(false);
    } else if (state == State::Activate) {
        setActivate(false);
    }

    WSurface::notifyEndState(state);
}

WAYLIB_SERVER_END_NAMESPACE
