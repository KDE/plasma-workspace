/*
 *   Copyright (C) 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License either version 2, or
 *   (at your option) any later version as published by the Free Software
 *   Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef FILEBROWSERENGINE_H
#define FILEBROWSERENGINE_H

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

    KDirWatch *m_dirWatch;
    void updateData(const QString &path, EventType event);
    void clearData(const QString &path);

    // QMap < QString, QStringList > m_regiteredListeners;
};

#endif
