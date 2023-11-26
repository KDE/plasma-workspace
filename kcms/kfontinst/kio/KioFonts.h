#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KIO/WorkerBase>
#include <sys/types.h>

class QTemporaryDir;
class QUrl;

namespace KFI
{
class Family;
class Style;
class FontInstInterface;

class CKioFonts : public KIO::WorkerBase
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

    KIO::WorkerResult listDir(const QUrl &url) override;
    KIO::WorkerResult put(const QUrl &url, int permissions, KIO::JobFlags flags) override;
    KIO::WorkerResult get(const QUrl &url) override;
    KIO::WorkerResult del(const QUrl &url, bool isFile) override;
    KIO::WorkerResult copy(const QUrl &src, const QUrl &dest, int mode, KIO::JobFlags flags) override;
    KIO::WorkerResult rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags) override;
    KIO::WorkerResult stat(const QUrl &url) override;
    KIO::WorkerResult special(const QByteArray &a) override;

private:
    void listFolder(KIO::UDSEntry &entry, EFolder folder);
    QString getUserName(uid_t uid);
    QString getGroupName(gid_t gid);
    bool createStatEntry(KIO::UDSEntry &entry, const QUrl &url, EFolder folder);
    void createUDSEntry(KIO::UDSEntry &entry, EFolder folder);
    bool createUDSEntry(KIO::UDSEntry &entry, const Family &family, const Style &style);
    Family getFont(const QUrl &url, EFolder folder);
    KIO::WorkerResult handleResp(int resp, const QString &file, const QString &tempFile = QString(), bool destIsSystem = false);

private:
    FontInstInterface *m_interface;
    QTemporaryDir *m_tempDir;
    QHash<uid_t, QString> m_userCache;
    QHash<gid_t, QString> m_groupCache;
};

}
