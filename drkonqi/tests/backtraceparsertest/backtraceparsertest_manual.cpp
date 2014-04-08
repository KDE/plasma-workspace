/*
    Copyright (C) 2010  George Kiagiadakis <kiagiadakis.george@gmail.com>

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
#include "fakebacktracegenerator.h"
#include "../../parser/backtraceparser.h"
#include <QtCore/QFile>
#include <QtCore/QSharedPointer>
#include <QtCore/QTextStream>
#include <QtCore/QMetaEnum>
#include <QtCore/QCoreApplication>
#include <QCommandLineParser>
#include <kaboutdata.h>
#include <KLocalizedString>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    KAboutData aboutData("backtraceparsertest_manual", QString(), i18n("backtraceparsertest_manual"), "1.0");
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption("debugger", i18n("The debugger name passed to the parser factory"), "name", "gdb"));
    parser.addPositionalArgument("file", i18n("A file containing the backtrace."), "[file]");
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    QString debugger = parser.value("debugger");
    if(parser.positionalArguments().isEmpty()) {
        parser.showHelp(1);
        return 1;
    }
    QString file = parser.positionalArguments().first();

    if (!QFile::exists(file)) {
        QTextStream(stderr) << "The specified file does not exist" << endl;
        return 1;
    }

    FakeBacktraceGenerator generator;
    QSharedPointer<BacktraceParser> btparser(BacktraceParser::newParser(debugger));
    btparser->connectToGenerator(&generator);
    generator.sendData(file);

    QMetaEnum metaUsefulness = BacktraceParser::staticMetaObject.enumerator(
                                BacktraceParser::staticMetaObject.indexOfEnumerator("Usefulness"));
    QTextStream(stdout) << "Usefulness: " << metaUsefulness.valueToKey(btparser->backtraceUsefulness()) << endl;
    QTextStream(stdout) << "First valid functions: " << btparser->firstValidFunctions().join(" ") << endl;
    QTextStream(stdout) << "Simplified backtrace:\n" << btparser->simplifiedBacktrace() << endl;
    QStringList l = static_cast<QStringList>(btparser->librariesWithMissingDebugSymbols().toList());
    QTextStream(stdout) << "Missing dbgsym libs: " << l.join(" ") << endl;

    return 0;
}
