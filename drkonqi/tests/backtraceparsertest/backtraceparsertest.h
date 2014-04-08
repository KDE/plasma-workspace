/*
    Copyright (C) 2009  George Kiagiadakis <gkiagia@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef BACKTRACEPARSERTEST_H
#define BACKTRACEPARSERTEST_H

#include <QtTest>
#include "fakebacktracegenerator.h"
#include "../../parser/backtraceparser.h"

class BacktraceParserTest : public QObject
{
    Q_OBJECT
public:
    BacktraceParserTest(QObject *parent = 0);

private Q_SLOTS:
    void btParserUsefulnessTest_data();
    void btParserUsefulnessTest();
    void btParserFunctionsTest_data();
    void btParserFunctionsTest();
    void btParserBenchmark_data();
    void btParserBenchmark();

private:
    void fetchData(const QString & group);

    QSettings m_settings;
    FakeBacktraceGenerator *m_generator;
};

#endif
