// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "output.h"
#include "qmlengine.h"
#include "surfacewrapper.h"
#include "wallpaperprovider.h"

#include <QQuickItem>

QmlEngine::QmlEngine(QObject *parent)
    : QQmlApplicationEngine(parent)
    , titleBarComponent(this, "Tinywl", "TitleBar")
    , decorationComponent(this, "Tinywl", "Decoration")
    , taskBarComponent(this, "Tinywl", "TaskBar")
    , surfaceContent(this, "Tinywl", "SurfaceContent")
    , shadowComponent(this, "Tinywl", "Shadow")
    , geometryAnimationComponent(this, "Tinywl", "GeometryAnimation")
{
}

QQuickItem *QmlEngine::createTitleBar(SurfaceWrapper *surface, QQuickItem *parent)
{
    auto context = qmlContext(parent);
    auto obj = titleBarComponent.beginCreate(context);
    titleBarComponent.setInitialProperties(obj, {
        {"surface", QVariant::fromValue(surface)}
    });
    auto item = qobject_cast<QQuickItem*>(obj);
    Q_ASSERT(item);
    item->setParent(parent);
    item->setParentItem(parent);
    titleBarComponent.completeCreate();

    return item;
}

QQuickItem *QmlEngine::createDecoration(SurfaceWrapper *surface, QQuickItem *parent)
{
    auto context = qmlContext(parent);
    auto obj = decorationComponent.beginCreate(context);
    decorationComponent.setInitialProperties(obj, {
        {"surface", QVariant::fromValue(surface)}
    });
    auto item = qobject_cast<QQuickItem*>(obj);
    Q_ASSERT(item);
    item->setParent(parent);
    item->setParentItem(parent);
    decorationComponent.completeCreate();

    return item;
}

QQuickItem *QmlEngine::createBorder(SurfaceWrapper *surface, QQuickItem *parent)
{
    auto context = qmlContext(parent);
    auto obj = borderComponent.beginCreate(context);
    borderComponent.setInitialProperties(obj, {
        {"surface", QVariant::fromValue(surface)}
    });
    auto item = qobject_cast<QQuickItem*>(obj);
    Q_ASSERT(item);
    item->setParent(parent);
    item->setParentItem(parent);
    borderComponent.completeCreate();

    return item;
}

QQuickItem *QmlEngine::createTaskBar(Output *output, QQuickItem *parent)
{
    auto context = qmlContext(parent);
    auto obj = taskBarComponent.beginCreate(context);
    taskBarComponent.setInitialProperties(obj, {
        {"output", QVariant::fromValue(output)}
    });
    auto item = qobject_cast<QQuickItem*>(obj);
    qDebug() << taskBarComponent.errorString();
    Q_ASSERT(item);
    item->setParent(parent);
    item->setParentItem(parent);
    taskBarComponent.completeCreate();

    return item;
}

QQuickItem *QmlEngine::createShadow(QQuickItem *parent)
{
    auto context = qmlContext(parent);
    auto obj = shadowComponent.beginCreate(context);
    auto item = qobject_cast<QQuickItem*>(obj);
    Q_ASSERT(item);
    item->setParent(parent);
    item->setParentItem(parent);
    shadowComponent.completeCreate();

    return item;
}

QQuickItem *QmlEngine::createGeometryAnimation(SurfaceWrapper *surface, QQuickItem *parent)
{
    auto context = qmlContext(parent);
    auto obj = geometryAnimationComponent.beginCreate(context);
    geometryAnimationComponent.setInitialProperties(obj, {
        {"surface", QVariant::fromValue(surface)}
    });
    auto item = qobject_cast<QQuickItem*>(obj);
    Q_ASSERT(item);
    item->setParent(parent);
    item->setParentItem(parent);
    geometryAnimationComponent.completeCreate();

    return item;
}

WallpaperImageProvider *QmlEngine::wallpaperImageProvider()
{
    Q_ASSERT(!this->imageProvider("wallpaper"));
    if (!wallpaperProvider) {
        wallpaperProvider = new WallpaperImageProvider;
        addImageProvider("wallpaper", wallpaperProvider);
    }

    return wallpaperProvider;
}
