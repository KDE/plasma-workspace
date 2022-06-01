/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "browsers/findprofile.h"
#include <QObject>
#include <memory>

class FakeFindProfile : public FindProfile
{
public:
    FakeFindProfile(const QList<Profile> &profiles)
        : m_profiles(profiles)
    {
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
public:
    explicit TestChromeBookmarks(QObject *parent = nullptr)
        : QObject(parent)
    {
    }
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
    std::unique_ptr<FakeFindProfile> m_findBookmarksInCurrentDirectory;
    QString m_configHome;
};
