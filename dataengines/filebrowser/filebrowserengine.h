/*
    SPDX-FileCopyrightText: 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Plasma/DataEngine>

class KDirWatch;

/**
 * This class evaluates the basic expressions given in the interface.
 */
class FileBrowserEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    FileBrowserEngine(QObject *parent, const QVariantList &args);
    ~FileBrowserEngine() override;

protected:
    bool sourceRequestEvent(const QString &path) override;
    void init();

protected Q_SLOTS:
    void dirDirty(const QString &path);
    void dirCreated(const QString &path);
    void dirDeleted(const QString &path);

private:
    enum EventType {
        INIT,
        DIRTY,
        CREATED,
        DELETED,
    };
    enum ObjectType {
        NOTHING,
        FILE,
        DIRECTORY,
    };

    KDirWatch *const m_dirWatch;
    void updateData(const QString &path, EventType event);
    void clearData(const QString &path);

    // QMap < QString, QStringList > m_regiteredListeners;
};
