/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>

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

#include <QCoreApplication>
#include <updatelaunchenvjob.h>

// This test syncs the current environment of the spawned process to systemd/whatever
// akin to dbus-update-activation-environment
// it can then be compared with "systemd-run --user -P env" or watched with dbus-monitor

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    auto job = new UpdateLaunchEnvJob(QProcessEnvironment::systemEnvironment());
    return job->exec();
}
