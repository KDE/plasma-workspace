/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "touchinputhelper.h"

#include <atomic>
#include <iostream>
#include <thread>

#include "qwayland-fake-input.h"
#include <QEventLoop>
#include <QGuiApplication>
#include <QRect>
#include <QTimer>
#include <QWaylandClientExtensionTemplate>

#include <abstracttasksmodel.h>
#include <tasksmodel.h>

namespace
{
class FakeInputInterface : public QWaylandClientExtensionTemplate<FakeInputInterface>, public QtWayland::org_kde_kwin_fake_input
{
public:
    FakeInputInterface()
        : QWaylandClientExtensionTemplate<FakeInputInterface>(5 /*ORG_KDE_KWIN_FAKE_INPUT_DESTROY_SINCE_VERSION*/)
    {
        initialize();
    }

    ~FakeInputInterface()
    {
        destroy();
    }
};

std::atomic<bool> s_locked{false}; // Used to block function return in the main thread
std::uint32_t s_touchId = 0;
QGuiApplication *s_application = nullptr;
wl_display *s_display = nullptr;
FakeInputInterface *s_fakeInputInterface = nullptr;
TaskManager::TasksModel *s_tasksModel = nullptr;
}

void init_application()
{
    if (s_application) {
        return;
    }
    s_locked = true;
    std::thread t([] {
        int argc = 1;
        const char *argv[] = {"touchinputhelper"};
        s_application = new QGuiApplication(argc, const_cast<char **>(argv));
        s_locked = false;
        s_locked.notify_one();
        std::cout << "QGuiApplication thread has started" << s_application << std::endl;
        s_application->exec();
    });
    t.detach();
    s_locked.wait(true);
}

void unload_application()
{
    if (!s_application) {
        return;
    }
    s_locked = true;
    QMetaObject::invokeMethod(s_application, [] {
        s_application->quit();
        s_application = nullptr;
        s_locked = false;
        s_locked.notify_one();
    });
    s_locked.wait(true);
    std::clog << "QGuiApplication quit" << std::endl;
}

void init_task_manager()
{
    if (s_tasksModel) {
        return;
    }
    s_locked = true;
    QMetaObject::invokeMethod(s_application, [] {
        s_tasksModel = new TaskManager::TasksModel(s_application);
        // Wait until the window appears in the model
        QEventLoop eventLoop;
        eventLoop.connect(s_tasksModel, &TaskManager::TasksModel::countChanged, &eventLoop, [&eventLoop] {
            if (s_tasksModel->rowCount() > 0) {
                eventLoop.quit();
            }
        });

        QTimer failSafeTimer{s_application};
        failSafeTimer.setInterval(5000);
        failSafeTimer.setSingleShot(true);
        eventLoop.connect(&failSafeTimer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
        failSafeTimer.start();

        if (s_tasksModel->rowCount() == 0) {
            eventLoop.exec();
        }
        s_locked = false;
        s_locked.notify_one();
        std::cout << "Task count " << s_tasksModel->rowCount() << std::endl;
    });
    s_locked.wait(true);
}

int get_task_count()
{
    return s_tasksModel ? s_tasksModel->rowCount() : -1;
}

int *get_window_rect(int row)
{
    int *result = (int *)calloc(4, sizeof(int));
    if (row < 0 || row >= s_tasksModel->rowCount()) {
        return result;
    }
    const QRect rect = s_tasksModel->index(row, 0).data(TaskManager::AbstractTasksModel::Geometry).toRect();
    result[0] = rect.x();
    result[1] = rect.y();
    result[2] = rect.width();
    result[3] = rect.height();
    return result;
}

int *get_screen_rect(int row)
{
    int *result = (int *)calloc(4, sizeof(int));
    if (row < 0 || row >= s_tasksModel->rowCount()) {
        return result;
    }
    const QRect rect = s_tasksModel->index(row, 0).data(TaskManager::AbstractTasksModel::ScreenGeometry).toRect();
    result[0] = rect.x();
    result[1] = rect.y();
    result[2] = rect.width();
    result[3] = rect.height();
    return result;
}

void maximize_window(int row)
{
    if (row < 0 || row >= s_tasksModel->rowCount() || s_tasksModel->index(row, 0).data(TaskManager::AbstractTasksModel::IsMaximized).toBool()) {
        return;
    }
    s_locked = true;
    QMetaObject::invokeMethod(s_application, [row] {
        s_tasksModel->requestToggleMaximized(s_tasksModel->index(row, 0));
        // Wait until the window is maximized
        QEventLoop eventLoop;
        eventLoop.connect(s_tasksModel, &TaskManager::TasksModel::dataChanged, &eventLoop, [&eventLoop, row] {
            if (s_tasksModel->index(row, 0).data(TaskManager::AbstractTasksModel::IsMaximized).toBool()) {
                eventLoop.quit();
            }
        });

        QTimer failSafeTimer{s_application};
        failSafeTimer.setInterval(5000);
        failSafeTimer.setSingleShot(true);
        eventLoop.connect(&failSafeTimer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
        failSafeTimer.start();

        if (!s_tasksModel->index(row, 0).data(TaskManager::AbstractTasksModel::IsMaximized).toBool()) {
            eventLoop.exec();
        }
        s_locked = false;
        s_locked.notify_one();
        std::cout << "Task is maximized" << std::endl;
    });
    s_locked.wait(true);
}

void init_fake_input()
{
    s_locked = true;
    QMetaObject::invokeMethod(s_application, [] {
        s_fakeInputInterface = new FakeInputInterface();
        s_fakeInputInterface->setParent(s_application);
        if (!s_fakeInputInterface->isInitialized()) {
            delete s_fakeInputInterface;
            s_fakeInputInterface = nullptr;
            std::cerr << "Failed to initialize fake_input" << std::endl;
            Q_UNREACHABLE(); // Crash directly to fail a test early
        }
        if (!s_fakeInputInterface->isActive()) {
            delete s_fakeInputInterface;
            s_fakeInputInterface = nullptr;
            std::cerr << "fake_input is inactive" << std::endl;
            Q_UNREACHABLE(); // Crash directly to fail a test early
        }
        s_fakeInputInterface->authenticate(QLatin1String("TouchInputHelper"), QLatin1String("used in a test"));
        s_display = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>()->display();
        wl_display_roundtrip(s_display);
        s_locked = false;
        s_locked.notify_one();
    });
    s_locked.wait(true);
}

void touch_down(int x, int y)
{
    s_locked = true;
    QMetaObject::invokeMethod(s_application, [x, y] {
        s_fakeInputInterface->touch_down(s_touchId, wl_fixed_from_int(x), wl_fixed_from_int(y));
        s_fakeInputInterface->touch_frame();
        wl_display_roundtrip(s_display);
        s_locked = false;
        s_locked.notify_one();
    });
    s_locked.wait(true);
    std::cout << "touch_down" << x << y << std::endl;
}

void touch_move(int x, int y)
{
    s_locked = true;
    QMetaObject::invokeMethod(s_application, [x, y] {
        s_fakeInputInterface->touch_motion(s_touchId, wl_fixed_from_int(x), wl_fixed_from_int(y));
        s_fakeInputInterface->touch_frame();
        wl_display_roundtrip(s_display);
        s_locked = false;
        s_locked.notify_one();
    });
    s_locked.wait(true);
    std::cout << "touch_move" << x << y << std::endl;
}

void touch_up()
{
    s_locked = true;
    QMetaObject::invokeMethod(s_application, [] {
        s_fakeInputInterface->touch_up(s_touchId++);
        s_fakeInputInterface->touch_frame();
        wl_display_roundtrip(s_display);
        s_locked = false;
        s_locked.notify_one();
    });
    s_locked.wait(true);
    std::cout << "touch_up" << std::endl;
}

#include "moc_touchinputhelper.cpp"
