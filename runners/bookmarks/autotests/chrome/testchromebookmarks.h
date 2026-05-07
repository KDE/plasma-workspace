/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "browsers/findprofile.h"
#include "favicon.h"
#include <QObject>
#include <memory>

/*
  Mock implementation of FindProfile for testing purposes.
  Manages the lifetime of profiles and their associated favicons to prevent memory leaks.
  */
class FakeFindProfile : public FindProfile
{
public:
    FakeFindProfile() = default;

    /**
     * @brief Adds a profile and takes ownership of its associated favicon.
     */
    void addProfile(const QString &path, const QString &name, std::unique_ptr<Favicon> icon)
    {
        m_profiles.append(Profile(path, name, std::move(icon)));
    }

    QList<Profile> find() override
    {
        return m_profiles;
    }

private:
    QList<Profile> m_profiles;
};

class TestChromeBookmarks : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void bookmarkFinderShouldFindEachProfileDirectory();
    void bookmarkFinderShouldReportNoProfilesOnErrors();
    void itShouldFindNothingWhenPrepareIsNotCalled();
    void itShouldGracefullyExitWhenFileIsNotFound();
    void itShouldFindAllBookmarks();
    void itShouldFindOnlyMatches();
    void itShouldClearResultAfterCallingTeardown();
    void itShouldFindBookmarksFromAllProfiles();

private:
    QString m_configHome;
};