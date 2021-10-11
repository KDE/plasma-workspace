#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KIO/SlaveBase>
#include <sys/types.h>

class QTemporaryDir;
class QUrl;

namespace KFI
{
class Family;
class Style;
class FontInstInterface;

class CKioFonts : public KIO::SlaveBase
{
public:
    enum EFolder {
        FOLDER_USER,
        FOLDER_SYS,
        FOLDER_ROOT,
        FOLDER_UNKNOWN,
    };

    CKioFonts(const QByteArray &pool, const QByteArray &app);
    ~CKioFonts() override;

    void listDir(const QUrl &url) override;
    void put(const QUrl &url, int permissions, KIO::JobFlags flags) override;
    void get(const QUrl &url) override;
    void del(const QUrl &url, bool isFile) override;
    void copy(const QUrl &src, const QUrl &dest, int mode, KIO::JobFlags flags) override;
    void rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags) override;
    void stat(const QUrl &url) override;
    void special(const QByteArray &a) override;

private:
    int listFolder(KIO::UDSEntry &entry, EFolder folder);
    QString getUserName(uid_t uid);
    QString getGroupName(gid_t gid);
    bool createStatEntry(KIO::UDSEntry &entry, const QUrl &url, EFolder folder);
    void createUDSEntry(KIO::UDSEntry &entry, EFolder folder);
    bool createUDSEntry(KIO::UDSEntry &entry, EFolder folder, const Family &family, const Style &style);
    Family getFont(const QUrl &url, EFolder folder);
    void handleResp(int resp, const QString &file, const QString &tempFile = QString(), bool destIsSystem = false);

private:
    FontInstInterface *m_interface;
    QTemporaryDir *m_tempDir;
    QHash<uid_t, QString> m_userCache;
    QHash<gid_t, QString> m_groupCache;
};

}
