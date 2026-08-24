/*
 * Copyright (c) 2026 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "rhythm/notemap.h"

#include <QtTest>

class TestNoteMap : public QObject
{
    Q_OBJECT

private:
    static QByteArray valid()
    {
        return R"({
            "name": "Xylophone",
            "sourceClip": "/clips/click.webm",
            "voices": 4,
            "notes": [
                { "note": 55, "track": "G4",  "rect": [454.067, 695.527, 108, 108, 1] },
                { "note": 57, "track": "A4",  "rect": [520.139, 703.757, 108, 108] },
                { "note": 60, "track": "C5",  "rect": [661.448, 704.38, 108, 108], "opacity": 0.5 }
            ]
        })";
    }

private slots:
    void parsesAValidMap()
    {
        NoteMap map;
        QString error;
        QVERIFY2(map.loadJson(valid(), &error), qPrintable(error));
        QVERIFY(map.isValid());
        QCOMPARE(map.name(), QStringLiteral("Xylophone"));
        QCOMPARE(map.sourceClip(), QStringLiteral("/clips/click.webm"));
        QCOMPARE(map.voices(), 4);
        QCOMPARE(map.count(), 3);
        QCOMPARE(map.notes(), QList<int>({55, 57, 60}));
    }

    void readsRectAndOpacity()
    {
        NoteMap map;
        QVERIFY(map.loadJson(valid()));

        const auto *g4 = map.entry(55);
        QVERIFY(g4);
        QCOMPARE(g4->track, QStringLiteral("G4"));
        QVERIFY(qAbs(g4->rect.x() - 454.067) < 1e-6);
        QVERIFY(qAbs(g4->rect.height() - 108.0) < 1e-6);
        QCOMPARE(g4->opacity, 1.0); // fifth rect element

        QCOMPARE(map.entry(57)->opacity, 1.0); // defaulted
        QCOMPARE(map.entry(60)->opacity, 0.5); // explicit field
    }

    void unknownNoteReturnsNull() { NoteMap map; QVERIFY(map.loadJson(valid())); QVERIFY(!map.entry(99)); }

    void voiceTrackNames()
    {
        QCOMPARE(NoteMap::trackName("G4", 0), QStringLiteral("G4"));
        QCOMPARE(NoteMap::trackName("G4", 1), QStringLiteral("G4b"));
        QCOMPARE(NoteMap::trackName("G4", 2), QStringLiteral("G4c"));
        // Distinct for every voice within a sane range.
        QSet<QString> seen;
        for (int voice = 0; voice < 8; voice++)
            seen.insert(NoteMap::trackName("G4", voice));
        QCOMPARE(seen.size(), 8);
    }

    void rejectsBadInput_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::newRow("not json") << QByteArray("this is not json");
        QTest::newRow("not an object") << QByteArray("[1,2,3]");
        QTest::newRow("no sourceClip")
            << QByteArray(R"({"notes":[{"note":1,"track":"a","rect":[0,0,1,1]}]})");
        QTest::newRow("no notes array") << QByteArray(R"({"sourceClip":"x"})");
        QTest::newRow("empty notes") << QByteArray(R"({"sourceClip":"x","notes":[]})");
        QTest::newRow("note out of range")
            << QByteArray(R"({"sourceClip":"x","notes":[{"note":200,"track":"a","rect":[0,0,1,1]}]})");
        QTest::newRow("missing track")
            << QByteArray(R"({"sourceClip":"x","notes":[{"note":1,"rect":[0,0,1,1]}]})");
        QTest::newRow("short rect")
            << QByteArray(R"({"sourceClip":"x","notes":[{"note":1,"track":"a","rect":[0,0]}]})");
        QTest::newRow("empty rect")
            << QByteArray(R"({"sourceClip":"x","notes":[{"note":1,"track":"a","rect":[0,0,0,0]}]})");
        QTest::newRow("duplicate note")
            << QByteArray(R"({"sourceClip":"x","notes":[
                   {"note":1,"track":"a","rect":[0,0,1,1]},
                   {"note":1,"track":"b","rect":[0,0,1,1]}]})");
    }

    void rejectsBadInput()
    {
        QFETCH(QByteArray, json);
        NoteMap map;
        QString error;
        QVERIFY(!map.loadJson(json, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!map.isValid());
        QCOMPARE(map.count(), 0);
    }

    void voicesAtLeastOne()
    {
        NoteMap map;
        QVERIFY(map.loadJson(
            R"({"sourceClip":"x","voices":0,"notes":[{"note":1,"track":"a","rect":[0,0,1,1]}]})"));
        QCOMPARE(map.voices(), 1);
    }

    void clearResetsState()
    {
        NoteMap map;
        QVERIFY(map.loadJson(valid()));
        map.clear();
        QVERIFY(!map.isValid());
        QCOMPARE(map.count(), 0);
        QVERIFY(map.sourceClip().isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestNoteMap)

#include "test_note_map.moc"
