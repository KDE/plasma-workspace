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
#include "autostart.h"

#include <KAboutData>
#include <KLocalizedString>

K_PLUGIN_CLASS_WITH_JSON(Autostart, "metadata.json")

Autostart::Autostart(QObject *parent, const QVariantList &)
    : KQuickAddons::ConfigModule(parent)
    , m_model(new AutostartModel(this))
{
    setButtons(Help);

    qmlRegisterUncreatableType<AutostartModel>("org.kde.plasma.kcm.autostart", 1, 0, "AutostartModel", QStringLiteral("Only for enums"));

    KAboutData *about = new KAboutData(QStringLiteral("kcm_autostart"),
                                       i18n("Autostart"),
                                       QStringLiteral("1.0"),
                                       i18n("Session Autostart Manager Control Panel Module"),
                                       KAboutLicense::GPL,
                                       i18n("Copyright © 2006–2020 Autostart Manager team"));
    about->addAuthor(i18n("Stephen Leaf"), QString(), QStringLiteral("smileaf@gmail.com"));
    about->addAuthor(i18n("Montel Laurent"), i18n("Maintainer"), QStringLiteral("montel@kde.org"));
    about->addAuthor(i18n("Nicolas Fella"), QString(), QStringLiteral("nicolas.fella@gmx.de"));
    setAboutData(about);
}

Autostart::~Autostart()
{
}

AutostartModel *Autostart::model() const
{
    return m_model;
}

void Autostart::load()
{
    m_model->load();
}

void Autostart::defaults()
{
}

void Autostart::save()
{
}

#include "autostart.moc"
