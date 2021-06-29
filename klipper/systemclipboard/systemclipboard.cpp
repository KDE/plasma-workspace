/*
    SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the Lesser GNU General Public License
    along with this program; see the file COPYING.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "systemclipboard.h"

#include "qtclipboard.h"
#include "waylandclipboard.h"

#include <KWindowSystem>
#include <QGuiApplication>

SystemClipboard *SystemClipboard::instance()
{
    if (!qApp || qApp->closingDown()) {
        return nullptr;
    }
    static SystemClipboard *systemClipboard = nullptr;
    if (!systemClipboard) {
        if (KWindowSystem::isPlatformWayland()) {
            systemClipboard = new WaylandClipboard(qApp);
        } else {
            systemClipboard = new QtClipboard(qApp);
        }
    }
    return systemClipboard;
}

SystemClipboard::SystemClipboard(QObject *parent)
    : QObject(parent)
{
}
