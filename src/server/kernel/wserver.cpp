/*
 * Copyright (C) 2021 zkyd
 *
 * Author:     zkyd <zkyd@zjide.org>
 *
 * Maintainer: zkyd <zkyd@zjide.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#define private public
#include <QCoreApplication>
#undef private

#include "wserver.h"
#include "wsurface.h"
#include "wthreadutils.h"
#include "wseat.h"
#include "platformplugin/qwlrootsintegration.h"

#include <qwdisplay.h>
#include <qwdatadevice.h>

#include <QVector>
#include <QThread>
#include <QEvent>
#include <QCoreApplication>
#include <QAbstractEventDispatcher>
#include <QSocketNotifier>
#include <QMutex>
#include <QDebug>
#include <private/qthread_p.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformthemefactory_p.h>
#include <qpa/qplatformintegrationfactory_p.h>
#include <qpa/qplatformtheme.h>

extern "C" {
#include <wlr/backend.h>
#define static
#include <wlr/render/wlr_renderer.h>
#undef static
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>
}

QW_USE_NAMESPACE
WAYLIB_SERVER_BEGIN_NAMESPACE

class WServerPrivate;
class ServerThread : public QDaemonThread
{
    Q_OBJECT
public:
    ServerThread(WServerPrivate *s)
        : server(s) {
        setObjectName(QT_STRINGIFY(WAYLIB_SERVER_NAMESPACE));
        Q_ASSERT(!objectName().isEmpty());
    }

    Q_SLOT void processWaylandEvents();

private:
    WServerPrivate *server;
    friend class WServer;
    friend class WServerPrivate;
};

class WServerPrivate : public WObjectPrivate
{
public:
    WServerPrivate(WServer *qq)
        : WObjectPrivate(qq)
    {
    }
    ~WServerPrivate()
    {
        Q_ASSERT_X(!thread || !thread->isRunning(), "WServer",
                   "Must stop the server before destroy it.");
        qDeleteAll(interfaceList);
    }

    void init();
    void stop();
    bool startThread();

    W_DECLARE_PUBLIC(WServer)
    QScopedPointer<ServerThread> thread;
    QScopedPointer<WThreadUtil> threadUtil;
    QScopedPointer<QSocketNotifier> sockNot;
    QScopedPointer<QObject> slotOwner;

    QVector<WServerInterface*> interfaceList;

    QWDisplay *display = nullptr;
    const char *socket = nullptr;
    wl_event_loop *loop = nullptr;
};

void WServerPrivate::init()
{
    Q_ASSERT(!display);

    slotOwner.reset(new QObject());
    display = new QWDisplay(q_func());

    // free follow display
    Q_UNUSED(QWDataDeviceManager::create(display));

    W_Q(WServer);

    for (auto i : qAsConst(interfaceList)) {
        i->create(q);
    }

    socket = display->addSocketAuto();
    if (!socket) {
        qFatal("Create socket failed");
    }

    loop = wl_display_get_event_loop(display->handle());
    int fd = wl_event_loop_get_fd(loop);

    sockNot.reset(new QSocketNotifier(fd, QSocketNotifier::Read));
    QObject::connect(sockNot.get(), &QSocketNotifier::activated,
                     thread.get(), &ServerThread::processWaylandEvents);

    QAbstractEventDispatcher *dispatcher = thread->eventDispatcher();
    QObject::connect(dispatcher, &QAbstractEventDispatcher::aboutToBlock,
                     thread.get(), &ServerThread::processWaylandEvents);

    QMetaObject::invokeMethod(q, &WServer::_started, Qt::QueuedConnection);
}

void WServerPrivate::stop()
{
    W_Q(WServer);

    slotOwner.reset();

    auto i = interfaceList.crbegin();
    for (; i != interfaceList.crend(); ++i) {
        if ((*i)->isValid()) {
            (*i)->destroy(q);
        }

        delete *i;
    }

    interfaceList.clear();
    sockNot.reset();
    thread->eventDispatcher()->disconnect(thread.get());

    if (display) {
        display->deleteLater();
        display = nullptr;
    }

    // delete children in server thread
    QObjectPrivate::get(q)->deleteChildren();
    thread->quit();
}

// In main thread
bool WServerPrivate::startThread()
{
    if (thread && thread->isRunning())
        return false;

    if (!thread) {
        thread.reset(new ServerThread(this));
        thread->moveToThread(thread.get());
        threadUtil.reset(new WThreadUtil(thread.get()));
        q_func()->moveToThread(thread.get());
        Q_ASSERT(q_func()->thread() == thread.get());
    }

    thread->start(QThread::HighestPriority);
    return true;
}

void ServerThread::processWaylandEvents()
{
    int ret = wl_event_loop_dispatch(server->loop, 0);
    if (ret)
        fprintf(stderr, "wl_event_loop_dispatch error: %d\n", ret);
    wl_display_flush_clients(server->display->handle());
}

WServer::WServer()
    : QObject()
    , WObject(*new WServerPrivate(this))
{
#ifdef QT_DEBUG
    wlr_log_init(WLR_DEBUG, NULL);
#else
    wlr_log_init(WLR_INFO, NULL);
#endif
}

WDisplayHandle *WServer::handle() const
{
    W_DC(WServer);
    return reinterpret_cast<WDisplayHandle*>(d->display);
}

void WServer::stop()
{
    W_D(WServer);

    Q_ASSERT(d->thread && d->display);
    Q_ASSERT(d->thread->isRunning());
    Q_ASSERT(QThread::currentThread() != d->thread.data());

    d->threadUtil->run(this, d, &WServerPrivate::stop);
    if (!d->thread->wait()) {
        d->thread->terminate();
    }
}

void WServer::attach(WServerInterface *interface)
{
    W_D(WServer);
    Q_ASSERT(!d->interfaceList.contains(interface));
    d->interfaceList << interface;

    Q_ASSERT(interface->m_server == nullptr);
    interface->m_server = this;

    if (isRunning()) {
        d->threadUtil->run(this, interface, &WServerInterface::create, this);
    }
}

bool WServer::detach(WServerInterface *interface)
{
    W_D(WServer);
    if (!d->interfaceList.contains(interface))
        return false;

    Q_ASSERT(interface->m_server == this);
    interface->m_server = nullptr;

    if (!isRunning())
        return false;

    d->threadUtil->run(this, interface, &WServerInterface::destroy, this);
    return true;
}

QVector<WServerInterface *> WServer::interfaceList() const
{
    W_DC(WServer);
    return d->interfaceList;
}

QVector<WServerInterface *> WServer::findInterfaces(void *handle) const
{
    QVector<WServerInterface*> list;
    Q_FOREACH(auto i, interfaceList()) {
        if (i->handle() == handle)
            list << i;
    }

    return list;
}

WServerInterface *WServer::findInterface(void *handle) const
{
    Q_FOREACH(auto i, interfaceList()) {
        if (i->handle() == handle)
            return i;
    }

    return nullptr;
}

WServer *WServer::fromThread(const QThread *thread)
{
    if (auto st = qobject_cast<const ServerThread*>(thread)) {
        return st->server->q_func();
    }

    return nullptr;
}

WServer *WServer::from(WServerInterface *interface)
{
    return interface->m_server;
}

static bool initializeQtPlatform(bool isMaster, const QStringList &parameters, std::function<void()> onInitialized)
{
    Q_ASSERT(QGuiApplication::instance() == nullptr);
    if (QGuiApplicationPrivate::platform_integration)
        return false;

    QGuiApplicationPrivate::platform_integration = new QWlrootsIntegration(isMaster, parameters, onInitialized);

    // for platform theme
    QStringList themeNames = QWlrootsIntegration::instance()->themeNames();

    if (!QGuiApplicationPrivate::platform_theme) {
        for (const QString &themeName : qAsConst(themeNames)) {
            QGuiApplicationPrivate::platform_theme = QPlatformThemeFactory::create(themeName);
            if (QGuiApplicationPrivate::platform_theme) {
                break;
            }
        }
    }

    if (!QGuiApplicationPrivate::platform_theme) {
        for (const QString &themeName : qAsConst(themeNames)) {
            QGuiApplicationPrivate::platform_theme = QWlrootsIntegration::instance()->createPlatformTheme(themeName);
            if (QGuiApplicationPrivate::platform_theme) {
                break;
            }
        }
    }

    // fallback
    if (!QGuiApplicationPrivate::platform_theme) {
        QGuiApplicationPrivate::platform_theme = new QPlatformTheme;
    }

    return true;
}

static QThread *test1(QObject *a) {
    return a->thread();
}

static QThread *test2() {
    return QThread::currentThread();
}

QFuture<void> WServer::start()
{
    W_D(WServer);

    if (!d->startThread())
        return {};

    return d->threadUtil->run(this, d, &WServerPrivate::init);
}

void WServer::initializeQPA(bool master, const QStringList &parameters)
{
    if (!initializeQtPlatform(master, parameters, nullptr)) {
        qFatal("Can't initialize Qt platform plugin.");
        return;
    }
}

void WServer::initializeProxyQPA(int &argc, char **argv, const QStringList &proxyPlatformPlugins, const QStringList &parameters)
{
    Q_ASSERT(!proxyPlatformPlugins.isEmpty());

    W_DC(WServer);
    Q_ASSERT(d->socket);
    qputenv("WAYLAND_DISPLAY", d->socket);
    QPlatformIntegration *proxy = nullptr;
    for (const QString &name : proxyPlatformPlugins) {
        if (name.isEmpty())
            continue;
        proxy = QPlatformIntegrationFactory::create(name, parameters, argc, argv);
        if (proxy)
            break;
    }
    if (!proxy) {
        qFatal() << "Can't create the proxy platform plugin:" << proxyPlatformPlugins;
    }
    proxy->initialize();
    QWlrootsIntegration::instance()->setProxy(proxy);
    qunsetenv("WAYLAND_DISPLAY");
}

bool WServer::waitForStoped(QDeadlineTimer deadline)
{
    if (!isRunning())
        return true;

    W_D(WServer);
    return d->thread->wait(deadline);
}

bool WServer::isRunning() const
{
    W_DC(WServer);
    return d->display && d->thread && d->thread->isRunning();
}

const char *WServer::displayName() const
{
    if (!isRunning())
        return nullptr;
    W_DC(WServer);
    return d->socket;
}

WThreadUtil *WServer::threadUtil() const
{
    W_DC(WServer);
    return d->threadUtil.get();
}

bool WServer::event(QEvent *e)
{
    if (e->type() != WInputEvent::type())
        return QObject::event(e);

    e->ignore();
    return true;
}

QObject *WServer::slotOwner() const
{
    W_DC(WServer);
    return d->slotOwner.get();
}

void WServer::_started()
{
    W_DC(WServer);

    Q_EMIT started();
}

WAYLIB_SERVER_END_NAMESPACE

#include "wserver.moc"
