/*
 *   Copyright 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
 *   Copyright 2012 Glenn Ergeerts <marco.gulino@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef TESTCHROMEBOOKMARKS_H
#define TESTCHROMEBOOKMARKS_H

#include <QObject>
#include "browsers/findprofile.h"

class FakeFindProfile : public FindProfile {
public:
  FakeFindProfile(const QList<Profile> &profiles) : m_profiles(profiles) {}
    QList<Profile> find() override { return m_profiles; }
private:
  QList<Profile> m_profiles;
};

class TestChromeBookmarks : public QObject
{
Q_OBJECT
public:
    explicit TestChromeBookmarks(QObject* parent = nullptr) : QObject(parent) {}
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
    QScopedPointer<FakeFindProfile> m_findBookmarksInCurrentDirectory;

};

#endif // TESTCHROMEBOOKMARKS_H
