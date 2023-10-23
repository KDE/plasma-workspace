/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#define QT_FORCE_ASSERTS

#include <atomic>
#include <iostream>
#include <thread>

#include <Python.h>

#include "qwayland-fake-input.h"
#include <QEventLoop>
#include <QGuiApplication>
#include <QRect>
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

    Q_DISABLE_COPY_MOVE(FakeInputInterface)
};

std::uint32_t s_touchId = 0;
QGuiApplication *s_application = nullptr;
wl_display *s_display = nullptr;
FakeInputInterface *s_fakeInputInterface = nullptr;
TaskManager::TasksModel *s_tasksModel = nullptr;

void init_fake_input()
{
    QMetaObject::invokeMethod(
        s_application,
        [] {
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
        },
        Qt::BlockingQueuedConnection);
}

PyObject *init_module(PyObject *, PyObject *)
{
    if (s_application) {
        Py_RETURN_NONE;
    }
    std::atomic<bool> locked{true}; // Used to block function return in the main thread
    std::thread t([&locked] {
        int argc = 1;
        const char *argv[] = {"inputsynth"};
        qputenv("QT_QPA_PLATFORM", "wayland");
        s_application = new QGuiApplication(argc, const_cast<char **>(argv));
        locked = false;
        locked.notify_one();
        std::clog << "QGuiApplication thread has started" << s_application << std::endl;
        s_application->exec();
    });
    t.detach();
    locked.wait(true);
    init_fake_input();
    Py_RETURN_NONE;
}

PyObject *init_task_manager(PyObject *, PyObject *)
{
    if (s_tasksModel) {
        Py_RETURN_NONE;
    }
    QMetaObject::invokeMethod(
        s_application,
        [] {
            s_tasksModel = new TaskManager::TasksModel(s_application);
            // Wait until the window appears in the model
            QEventLoop eventLoop(s_application);
            eventLoop.connect(s_tasksModel, &TaskManager::TasksModel::countChanged, &eventLoop, [&eventLoop] {
                if (s_tasksModel->rowCount() > 0) {
                    eventLoop.quit();
                }
            });

            if (s_tasksModel->rowCount() == 0) {
                eventLoop.exec();
            }
            std::clog << "Task count " << s_tasksModel->rowCount() << std::endl;
        },
        Qt::BlockingQueuedConnection);
    Py_RETURN_NONE;
}

void unload_module(void *)
{
    if (!s_application) {
        return;
    }
    QMetaObject::invokeMethod(
        s_application,
        [] {
            s_application->quit();
            s_application = nullptr;
        },
        Qt::BlockingQueuedConnection);
    std::clog << "QGuiApplication quit" << std::endl;
}

PyObject *get_task_count(PyObject *, PyObject *)
{
    return PyLong_FromLong(s_tasksModel ? s_tasksModel->rowCount() : -1);
}

PyObject *get_window_rect(PyObject *, PyObject *arg)
{
    const long row = PyLong_AsLong(arg);
    if (row < 0 || row >= s_tasksModel->rowCount()) {
        Py_RETURN_NONE;
    }
    PyObject *result = PyTuple_New(4);
    const QRect rect = s_tasksModel->index(row, 0).data(TaskManager::AbstractTasksModel::Geometry).toRect();
    PyTuple_SetItem(result, 0, PyLong_FromLong(rect.x()));
    PyTuple_SetItem(result, 1, PyLong_FromLong(rect.y()));
    PyTuple_SetItem(result, 2, PyLong_FromLong(rect.width()));
    PyTuple_SetItem(result, 3, PyLong_FromLong(rect.height()));
    return result;
}

PyObject *get_screen_rect(PyObject *, PyObject *arg)
{
    const long row = PyLong_AsLong(arg);
    if (row < 0 || row >= s_tasksModel->rowCount()) {
        Py_RETURN_NONE;
    }
    PyObject *result = PyTuple_New(4);
    const QRect rect = s_tasksModel->index(row, 0).data(TaskManager::AbstractTasksModel::ScreenGeometry).toRect();
    PyTuple_SetItem(result, 0, PyLong_FromLong(rect.x()));
    PyTuple_SetItem(result, 1, PyLong_FromLong(rect.y()));
    PyTuple_SetItem(result, 2, PyLong_FromLong(rect.width()));
    PyTuple_SetItem(result, 3, PyLong_FromLong(rect.height()));
    return result;
}

PyObject *maximize_window(PyObject *, PyObject *arg)
{
    const long row = PyLong_AsLong(arg);
    if (row < 0 || row >= s_tasksModel->rowCount() || s_tasksModel->index(row, 0).data(TaskManager::AbstractTasksModel::IsMaximized).toBool()) {
        Py_RETURN_NONE;
    }
    QMetaObject::invokeMethod(s_application, [row] {
        s_tasksModel->requestToggleMaximized(s_tasksModel->index(row, 0));
        std::clog << "Task is maximized" << std::endl;
    });
    Py_RETURN_NONE;
}

PyObject *is_window_maximized(PyObject *, PyObject *arg)
{
    const long row = PyLong_AsLong(arg);
    if (row < 0 || row >= s_tasksModel->rowCount()) {
        return PyBool_FromLong(0);
    }
    return PyBool_FromLong(s_tasksModel->index(row, 0).data(TaskManager::AbstractTasksModel::IsMaximized).toBool() ? 1 : 0);
}

PyObject *touch_down(PyObject *, PyObject *args)
{
    int x, y;
    PyArg_ParseTuple(args, "ii", &x, &y);
    QMetaObject::invokeMethod(
        s_application,
        [x, y] {
            s_fakeInputInterface->touch_down(s_touchId, wl_fixed_from_int(x), wl_fixed_from_int(y));
            s_fakeInputInterface->touch_frame();
            wl_display_roundtrip(s_display);
        },
        Qt::BlockingQueuedConnection);
    std::clog << "touch_down" << x << " " << y << std::endl;
    Py_RETURN_NONE;
}

PyObject *touch_move(PyObject *, PyObject *args)
{
    int x, y;
    PyArg_ParseTuple(args, "ii", &x, &y);
    QMetaObject::invokeMethod(s_application, [x, y] {
        s_fakeInputInterface->touch_motion(s_touchId, wl_fixed_from_int(x), wl_fixed_from_int(y));
        s_fakeInputInterface->touch_frame();
        wl_display_roundtrip(s_display);
        // Don't block here, otherwise the DBus interface may not work properly.
    });
    std::clog << "touch_move" << x << " " << y << std::endl;
    Py_RETURN_NONE;
}

PyObject *touch_up(PyObject *, PyObject *)
{
    QMetaObject::invokeMethod(
        s_application,
        [] {
            s_fakeInputInterface->touch_up(s_touchId++);
            s_fakeInputInterface->touch_frame();
            wl_display_roundtrip(s_display);
        },
        Qt::BlockingQueuedConnection);
    std::clog << "touch_up" << std::endl;
    Py_RETURN_NONE;
}

// Exported methods are collected in a table
// https://docs.python.org/3/c-api/structures.html
PyMethodDef method_table[] = {
    {"init_module", (PyCFunction)init_module, METH_NOARGS, "Initialize QGuiApplication"},
    {"init_task_manager", (PyCFunction)init_task_manager, METH_NOARGS, "Initialize the task manager model"},
    {"get_task_count", (PyCFunction)get_task_count, METH_NOARGS, "Get the number of running windows"},
    {"get_window_rect", (PyCFunction)get_window_rect, METH_O, "Get the geometry of the window at the specific index in a tuple"},
    {"get_screen_rect", (PyCFunction)get_screen_rect, METH_O, "Get the screen geometry of the window at the specific index in a tuple"},
    {"maximize_window", (PyCFunction)maximize_window, METH_O, "Maximize the window at the specific index"},
    {"is_window_maximized", (PyCFunction)is_window_maximized, METH_O, "A window is maximized"},
    {"touch_down", (PyCFunction)touch_down, METH_VARARGS, "Create a touch point"},
    {"touch_move", (PyCFunction)touch_move, METH_VARARGS, "Move a touch point"},
    {"touch_up", (PyCFunction)touch_up, METH_NOARGS, "Release a touch point"},
    {nullptr, nullptr, 0, nullptr} // Sentinel value ending the table
};

// A struct contains the definition of a module
PyModuleDef inputsynth_module = {
    PyModuleDef_HEAD_INIT,
    "inputsynth", // Module name
    "Python bindings of the fake input protocol",
    -1, // Optional size of the module state memory
    method_table,
    nullptr, // Optional slot definitions
    nullptr, // Optional traversal function
    nullptr, // Optional clear function
    unload_module // Optional module deallocation function
};
}

// The module init function
PyMODINIT_FUNC PyInit_inputsynth(void)
{
    PyObject *m = PyModule_Create(&inputsynth_module);
    if (!m) {
        Q_UNREACHABLE();
    }
    return m;
}
