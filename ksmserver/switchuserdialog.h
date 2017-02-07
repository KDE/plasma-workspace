/*
 *   Copyright 2015 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License or (at your option) version 3 or any later version
 *   accepted by the membership of KDE e.V. (or its successor approved
 *   by the membership of KDE e.V.), which shall act as a proxy
 *   defined in Section 14 of version 3 of the license.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SWITCHUSERDIALOG_H
#define SWITCHUSERDIALOG_H

#include <QQuickView>

class KDisplayManager;

namespace KWayland
{
namespace Client
{
class PlasmaShell;
class PlasmaShellSurface;
}
}

class KSMSwitchUserDialog : public QQuickView
{
    Q_OBJECT

public:
    explicit KSMSwitchUserDialog(KDisplayManager *dm, KWayland::Client::PlasmaShell *plasmaShell = nullptr, QWindow *parent = nullptr);
    ~KSMSwitchUserDialog() override = default;

    void init();

signals:
    void dismissed();

protected:
    bool event(QEvent *e) override;

private slots:
    void ungrab();

private:
    void setupWaylandIntegration();

    KDisplayManager *m_displayManager = nullptr;

    KWayland::Client::PlasmaShell *m_waylandPlasmaShell;
    KWayland::Client::PlasmaShellSurface *m_shellSurface = nullptr;

};

#endif // SWITCHUSERDIALOG_H
