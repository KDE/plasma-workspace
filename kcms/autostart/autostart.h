/*
    SPDX-FileCopyrightText: 2006-2007 Stephen Leaf
    smileaf@gmail.com
    SPDX-FileCopyrightText: 2008 Montel Laurent <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef AUTOSTART_H
#define AUTOSTART_H

#include <KQuickAddons/ConfigModule>

#include "autostartmodel.h"

class Autostart : public KQuickAddons::ConfigModule
{
    Q_OBJECT
    Q_PROPERTY(AutostartModel *model READ model CONSTANT)

public:
    explicit Autostart(QObject *parent, const QVariantList &);
    ~Autostart() override;

    void load() override;
    void save() override;
    void defaults() override;

    AutostartModel *model() const;

private:
    AutostartModel *m_model;
};

#endif
