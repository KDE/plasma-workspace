/*
  This file is part of the KDE project.

  Copyright (c) 2011 Lionel Chauvin <megabigbug@yahoo.fr>
  Copyright (c) 2011,2012 Cédric Bellegarde <gnumdk@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#ifndef KDBUSMENUIMPORTER_H
#define KDBUSMENUIMPORTER_H

#include "gtkicons.h"

#include <KIcon>
#include <KIconLoader>

#include <QDBusArgument>

#include <dbusmenuimporter.h>

class KDBusMenuImporter : public DBusMenuImporter
{

public:
    KDBusMenuImporter(WId wid, const QString &service, GtkIcons *icons, const QString &path, QObject *parent)
    : DBusMenuImporter(service, path, parent)
    , m_service(service)
    , m_path(path)
    , m_WId(wid)
    {
        m_icons = icons;
    }

    QString service() const { return m_service; }
    QString path() const { return m_path; }
    WId wid() const { return m_WId; }

protected:
    QIcon iconForName(const QString &name)
    override {
        if(m_icons->contains(name)){
            return QIcon::fromTheme(m_icons->value(name));
        }
        else if(!KIconLoader::global()->iconPath(name, 1, true ).isNull()){
            return QIcon::fromTheme(name);
        }
        return QIcon();
    }

private:
    GtkIcons *m_icons;
    QString m_service;
    QString m_path;
    WId m_WId;
};

#endif //KDBUSMENUIMPORTER_H