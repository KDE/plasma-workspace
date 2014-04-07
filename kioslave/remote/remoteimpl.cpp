/* This file is part of the KDE project
   Copyright (c) 2004 Kevin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "remoteimpl.h"

#include <kdebug.h>
#include <kglobalsettings.h>
#include <kstandarddirs.h>
#include <kdesktopfile.h>
#include <kservice.h>
#include <kglobal.h>
#include <klocale.h>
#include <kconfiggroup.h>

#include <QDir>
#include <QFile>

#include <sys/stat.h>

#define WIZARD_URL "remote:/x-wizard_service.desktop"
#define WIZARD_SERVICE "knetattach"

RemoteImpl::RemoteImpl()
{
	KGlobal::dirs()->addResourceType("remote_entries", "data", "remoteview");

	const QString path = KGlobal::dirs()->saveLocation("remote_entries");

	QDir dir = path;
	if (!dir.exists())
	{
		dir.cdUp();
		dir.mkdir("remoteview");
	}
}

void RemoteImpl::listRoot(KIO::UDSEntryList &list) const
{
	kDebug(1220) << "RemoteImpl::listRoot";

	QStringList names_found;
	const QStringList dirList = KGlobal::dirs()->resourceDirs("remote_entries");

	QStringList::ConstIterator dirpath = dirList.constBegin();
	const QStringList::ConstIterator end = dirList.constEnd();
	for(; dirpath!=end; ++dirpath)
	{
		QDir dir = *dirpath;
		if (!dir.exists()) continue;

		const QStringList filenames
			= dir.entryList( QDir::Files | QDir::Readable );


		KIO::UDSEntry entry;

		QStringList::ConstIterator name = filenames.constBegin();
		QStringList::ConstIterator endf = filenames.constEnd();

		for(; name!=endf; ++name)
		{
			if (!names_found.contains(*name))
			{
				entry.clear();
				createEntry(entry, *dirpath, *name);
				list.append(entry);
				names_found.append(*name);
			}
		}
	}
}

bool RemoteImpl::findDirectory(const QString &filename, QString &directory) const
{
	kDebug(1220) << "RemoteImpl::findDirectory";

	const QStringList dirList = KGlobal::dirs()->resourceDirs("remote_entries");

	QStringList::ConstIterator dirpath = dirList.constBegin();
	const QStringList::ConstIterator end = dirList.constEnd();
	for(; dirpath!=end; ++dirpath)
	{
		QDir dir = *dirpath;
		if (!dir.exists()) continue;

		QStringList filenames
			= dir.entryList( QDir::Files | QDir::Readable );


		KIO::UDSEntry entry;

		QStringList::ConstIterator name = filenames.constBegin();
		QStringList::ConstIterator endf = filenames.constEnd();

		for(; name!=endf; ++name)
		{
			if (*name==filename)
			{
				directory = *dirpath;
				return true;
			}
		}
	}

	return false;
}

QString RemoteImpl::findDesktopFile(const QString &filename) const
{
	kDebug(1220) << "RemoteImpl::findDesktopFile";

	QString directory;
	if (findDirectory(filename+".desktop", directory))
	{
		return directory+filename+".desktop";
	}

	return QString();
}

KUrl RemoteImpl::findBaseURL(const QString &filename) const
{
	kDebug(1220) << "RemoteImpl::findBaseURL";

	const QString file = findDesktopFile(filename);
	if (!file.isEmpty())
	{
		KDesktopFile desktop( file );
		return desktop.readUrl();
	}

	return KUrl();
}


void RemoteImpl::createTopLevelEntry(KIO::UDSEntry &entry) const
{
    entry.clear();
    entry.insert( KIO::UDSEntry::UDS_NAME, QString::fromLatin1("."));
    entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.insert( KIO::UDSEntry::UDS_ACCESS, 0777);
    entry.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1("inode/directory"));
    entry.insert( KIO::UDSEntry::UDS_ICON_NAME, QString::fromLatin1("folder-remote"));
    entry.insert( KIO::UDSEntry::UDS_USER, QString::fromLatin1("root"));
    entry.insert( KIO::UDSEntry::UDS_GROUP, QString::fromLatin1("root"));
}

static KUrl findWizardRealURL()
{
	KUrl url;
	KService::Ptr service = KService::serviceByDesktopName(WIZARD_SERVICE);

	if (service && service->isValid())
	{
		url.setPath( KStandardDirs::locate("apps",
				    service->entryPath())
				);
	}

	return url;
}

bool RemoteImpl::createWizardEntry(KIO::UDSEntry &entry) const
{
	entry.clear();

	KUrl url = findWizardRealURL();

	if (!url.isValid())
	{
		return false;
	}

    entry.insert( KIO::UDSEntry::UDS_NAME, i18n("Add Network Folder"));
    entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
    entry.insert( KIO::UDSEntry::UDS_URL, QString::fromLatin1(WIZARD_URL) );
    entry.insert( KIO::UDSEntry::UDS_LOCAL_PATH, url.path());
    entry.insert( KIO::UDSEntry::UDS_ACCESS, 0500);
    entry.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1("application/x-desktop"));
    entry.insert( KIO::UDSEntry::UDS_ICON_NAME, QString::fromLatin1("folder-new"));

	return true;
}

bool RemoteImpl::isWizardURL(const KUrl &url) const
{
	return url==KUrl(WIZARD_URL);
}


void RemoteImpl::createEntry(KIO::UDSEntry &entry,
                             const QString &directory,
                             const QString &file) const
{
	kDebug(1220) << "RemoteImpl::createEntry";

	KDesktopFile desktop(directory+file);

	kDebug(1220) << "path = " << directory << file;

	entry.clear();

	QString new_filename = file;
	new_filename.truncate( file.length()-8);

    entry.insert( KIO::UDSEntry::UDS_NAME, desktop.readName());
    entry.insert( KIO::UDSEntry::UDS_URL, "remote:/"+new_filename);

    entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1("inode/directory"));

    const QString icon = desktop.readIcon();
    entry.insert( KIO::UDSEntry::UDS_ICON_NAME, icon);
    entry.insert( KIO::UDSEntry::UDS_LINK_DEST, desktop.readUrl());
    entry.insert( KIO::UDSEntry::UDS_TARGET_URL, desktop.readUrl());
}

bool RemoteImpl::statNetworkFolder(KIO::UDSEntry &entry, const QString &filename) const
{
	kDebug(1220) << "RemoteImpl::statNetworkFolder: " << filename;

	QString directory;
	if (findDirectory(filename+".desktop", directory))
	{
		createEntry(entry, directory, filename+".desktop");
		return true;
	}

	return false;
}

bool RemoteImpl::deleteNetworkFolder(const QString &filename) const
{
	kDebug(1220) << "RemoteImpl::deleteNetworkFolder: " << filename;

	QString directory;
	if (findDirectory(filename+".desktop", directory))
	{
		kDebug(1220) << "Removing " << directory << filename << ".desktop";
		return QFile::remove(directory+filename+".desktop");
	}

	return false;
}

bool RemoteImpl::renameFolders(const QString &src, const QString &dest,
                               bool overwrite) const
{
	kDebug(1220) << "RemoteImpl::renameFolders: "
	          << src << ", " << dest << endl;

	QString directory;
	if (findDirectory(src+".desktop", directory))
	{
		if (!overwrite && QFile::exists(directory+dest+".desktop"))
		{
			return false;
		}

		kDebug(1220) << "Renaming " << directory << src << ".desktop";
		QDir dir(directory);
		bool res = dir.rename(src+".desktop", dest+".desktop");
		if (res)
		{
			KDesktopFile desktop(directory+dest+".desktop");
			desktop.desktopGroup().writeEntry("Name", dest);
		}
		return res;
	}

	return false;
}

bool RemoteImpl::changeFolderTarget(const QString &src, const QString &target,
                                    bool overwrite) const
{
	kDebug(1220) << "RemoteImpl::changeFolderTarget: "
	          << src << ", " << target << endl;

	QString directory;
	if (findDirectory(src+".desktop", directory))
	{
		if (!overwrite || !QFile::exists(directory+src+".desktop"))
		{
			return false;
		}

		kDebug(1220) << "Changing target " << directory << src << ".desktop";
		KDesktopFile desktop(directory+src+".desktop");
		desktop.desktopGroup().writeEntry("URL", target);
		return true;
	}

	return false;
}

