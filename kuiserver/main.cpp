/*
  * This file is part of the KDE project
  * Copyright (C) 2009 Shaun Reich <shaun.reich@kdemail.net>
  * Copyright (C) 2006-2008 Rafael Fernández López <ereslibre@kde.org>
  * Copyright (C) 2001 George Staikos <staikos@kde.org>
  * Copyright (C) 2000 Matej Koss <koss@miesto.sk>
  *                    David Faure <faure@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
*/

#include "uiserver.h"
#include "uiserver_p.h"
#include "progresslistmodel.h"

#include <kdbusservice.h>

#include <QCommandLineParser>

Q_LOGGING_CATEGORY(KUISERVER, "kuiserver")

extern "C" Q_DECL_EXPORT int kdemain(int argc, char **argv)
{
    QLoggingCategory::setFilterRules(QStringLiteral("kuiserver.debug = true"));

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kuiserver"));
    app.setApplicationVersion(QStringLiteral("2.0"));
    app.setOrganizationDomain(QStringLiteral("kde.org"));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    ProgressListModel model;
    KDBusService service(KDBusService::Unique);

    return app.exec();
}
