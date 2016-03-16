/*
 *   Copyright (C) 2013 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
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

#ifndef SHELLMANAGER_H
#define SHELLMANAGER_H

#include <QObject>

namespace Plasma {
    class Corona;
}

/**
 * ShellManager creates a ShellCorona instance and manages it.
 *
 * Shell manager loads "handlers" from QML files which suggests which shell
 * corona should currently be active.
 * For example switching between tablet and desktop shells on hardware changes.
 *
 * This class also provides crash handling.
 */

class ShellManager: public QObject {
    Q_OBJECT
public:
    static ShellManager * instance();
    ~ShellManager() override;

    static bool s_forceWindowed;
    static bool s_standaloneOption;
    static QString s_fixedShell;
    static QString s_testModeLayout;

    Plasma::Corona* corona() const;

protected Q_SLOTS:
    void registerHandler(QObject * handler);
    void deregisterHandler(QObject * handler);

public Q_SLOTS:
    void requestShellUpdate();
    void updateShell();

Q_SIGNALS:
    void shellChanged(const QString & shell);

private Q_SLOTS:
    void loadHandlers();

private:
    ShellManager();

    class Private;
    const QScopedPointer<Private> d;
};

#endif /* SHELLMANAGER_H */

