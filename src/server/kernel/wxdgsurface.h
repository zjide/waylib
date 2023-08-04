// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <WSurface>
#include <qwglobal.h>

QW_BEGIN_NAMESPACE
class QWSurface;
class QWXdgSurface;
QW_END_NAMESPACE

WAYLIB_SERVER_BEGIN_NAMESPACE

class WXdgSurfacePrivate;
class WAYLIB_SERVER_EXPORT WXdgSurface : public WSurface
{
    Q_OBJECT
    W_DECLARE_PRIVATE(WXdgSurface)
    Q_PROPERTY(bool isPopup READ isPopup CONSTANT)

public:
    explicit WXdgSurface(QW_NAMESPACE::QWXdgSurface *handle, WServer *server, QObject *parent = nullptr);
    ~WXdgSurface();

    static Type *toplevelType();
    static Type *popupType();
    static Type *noneType();
    Type *type() const override;
    bool isPopup() const;

    bool testAttribute(Attribute attr) const override;

    QW_NAMESPACE::QWXdgSurface *handle() const;
    QW_NAMESPACE::QWSurface *inputTargetAt(QPointF &localPos) const override;

    static WXdgSurface *fromHandle(QW_NAMESPACE::QWXdgSurface *handle);

    bool inputRegionContains(const QPointF &localPos) const override;
    WSurface *parentSurface() const override;

    bool resizeing() const;
    QPointF position() const override;
    QRect getContentGeometry() const override;

public Q_SLOTS:
    void setResizeing(bool resizeing);
    void setMaximize(bool on);
    void setActivate(bool on);
    void resize(const QSize &size) override;

protected:
    void notifyChanged(ChangeType type, std::any oldValue, std::any newValue) override;
    void notifyBeginState(State state) override;
    void notifyEndState(State state) override;
};

WAYLIB_SERVER_END_NAMESPACE
