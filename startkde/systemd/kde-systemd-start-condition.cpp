/*
  Copyright (c) 2020 Henri Chain <henri.chain@enioka.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <kautostart.h>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Checks start condition for a KDE systemd service"));
    parser.addHelpOption();
    QCommandLineOption option{QStringLiteral("condition"),
                              QStringLiteral("start condition, in the format 'rcfile:group:entry:default'."),
                              QStringLiteral("condition")};
    parser.addOption(option);
    parser.process(app);

    if (!parser.isSet(option)) {
        parser.showHelp(255);
    }

    if (KAutostart::isStartConditionMet(parser.value(option))) {
        return 0;
    } else {
        return 1;
    }
}
