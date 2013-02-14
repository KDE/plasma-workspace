/*
 *  Copyright 2012 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QApplication>
#include <QtQuick/QQuickView>
#include <QtCore/QDebug>
//#include "svg.h"

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocalizedstring.h>



#include "desktopcorona.h"
#include <plasma/containment.h>

static const char description[] = "Plasma2 library tests";
static const char version[] = "1.0";

int main(int argc, char** argv)
{

    KAboutData aboutData("testplasma2", 0, ki18n("Plasma2 test app"),
                         version, ki18n(description), KAboutData::License_GPL,
                         ki18n("Copyright 2012, The KDE Team"));
    aboutData.addAuthor(ki18n("Marco Martin"),
                        ki18n("Author and maintainer"),
                        "mart@kde.org");
    KCmdLineArgs::init(argc, argv, &aboutData);

    QApplication app(argc, argv);

    
    DesktopCorona *corona = new DesktopCorona();
    corona->initializeLayout();
    corona->checkScreens();
    
    return app.exec();
}
