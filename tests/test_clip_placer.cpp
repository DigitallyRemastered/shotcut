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

#include "rhythm/clipplacer.h"
#include "rhythm/midisource.h"

#include <QtTest>

namespace {

void appendU16(QByteArray &out, quint16 v)
{
    out.append(char(v >> 8));
    out.append(char(v & 0xff));
}
void appendU32(QByteArray &out, quint32 v)
{
    for (int shift = 24; shift >= 0; shift -= 8)
        out.append(char((v >> shift) & 0xff));
}
QByteArray vlq(quint32 value)
{
    QByteArray out;
    out.prepend(char(value & 0x7f));
    value >>= 7;
    while (value) {
        out.prepend(char((value & 0x7f) | 0x80));
        value >>= 7;
    }
    return out;
}

//! 96 ticks per quarter at the default 120 BPM: one quarter is 0.5 s.
QByteArray midiOf(const QVector<std::tuple<quint32, int, int>> &deltaChannelNote)
{
    QByteArray events;
    for (const auto &entry : deltaChannelNote) {
        events += vlq(std::get<0>(entry));
        events.append(char(0x90 | (std::get<1>(entry) & 0x0f)));
        events.append(char(std::get<2>(entry)));
        events.append(char(100));
    }
    events += vlq(0);
    events.append(char(0xff));
    events.append(char(0x2f));
    events.append(char(0x00));

    QByteArray out("MThd");
    appendU32(out, 6);
    appendU16(out, 0);
    appendU16(out, 1);
    appendU16(out, 96);
    out += "MTrk";
    appendU32(out, quint32(events.size()));
    return out + events;
}

QByteArray mapJson(int voices)
{
    return QStringLiteral(R"({
        "sourceClip": "/clips/click.webm",
        "voices": %1,
        "notes": [
            { "note": 60, "track": "C5", "rect": [10, 20, 100, 100] },
            { "note": 62, "track": "D5", "rect": [30, 40, 100, 100, 0.5] }
        ]
    })").arg(voices).toUtf8();
}

} // namespace

class TestClipPlacer : public QObject
{
    Q_OBJECT

private:
    NoteMap m_map;
    MidiSource m_midi;

    ClipPlacer::Request request(int lengthFrames = 1000, int clipFrames = 10)
    {
        ClipPlacer::Request r;
        r.midi = &m_midi;
        r.fps = 25.0;
        r.lengthFrames = lengthFrames;
        r.clipFrames = clipFrames;
        return r;
    }

private slots:
    void placesOneClipPerNote()
    {
        QVERIFY(m_map.loadJson(mapJson(1)));
        QVERIFY(m_midi.loadData(midiOf({{0, 0, 60}, {96, 0, 62}, {96, 0, 60}})));

        const auto plan = ClipPlacer::plan(m_map, request());
        QCOMPARE(plan.placements.size(), 3);
        QCOMPARE(plan.placements.at(0).frame, 0);
        QCOMPARE(plan.placements.at(1).frame, 13); // 0.5 s * 25 fps rounds to 13
        QCOMPARE(plan.placements.at(2).frame, 25);
        QCOMPARE(plan.placements.at(0).note, 60);
        QVERIFY(plan.unmappedNotes.isEmpty());
    }

    void carriesRectAndOpacityFromTheMap()
    {
        QVERIFY(m_map.loadJson(mapJson(1)));
        QVERIFY(m_midi.loadData(midiOf({{0, 0, 62}})));
        const auto plan = ClipPlacer::plan(m_map, request());
        QCOMPARE(plan.placements.size(), 1);
        QCOMPARE(plan.placements.at(0).rect, QRectF(30, 40, 100, 100));
        QCOMPARE(plan.placements.at(0).opacity, 0.5);
    }

    void repeatsRoundRobinAcrossVoices()
    {
        QVERIFY(m_map.loadJson(mapJson(3)));
        // The same note five times.
        QVERIFY(m_midi.loadData(
            midiOf({{0, 0, 60}, {24, 0, 60}, {24, 0, 60}, {24, 0, 60}, {24, 0, 60}})));

        const auto plan = ClipPlacer::plan(m_map, request());
        QCOMPARE(plan.placements.size(), 5);
        QCOMPARE(plan.placements.at(0).voice, 0);
        QCOMPARE(plan.placements.at(1).voice, 1);
        QCOMPARE(plan.placements.at(2).voice, 2);
        QCOMPARE(plan.placements.at(3).voice, 0); // wraps
        QCOMPARE(plan.placements.at(0).track, QStringLiteral("C5"));
        QCOMPARE(plan.placements.at(1).track, QStringLiteral("C5b"));
        QCOMPARE(plan.placements.at(2).track, QStringLiteral("C5c"));
        QCOMPARE(plan.tracks().size(), 3);
    }

    void differentNotesKeepSeparateVoiceCounters()
    {
        QVERIFY(m_map.loadJson(mapJson(2)));
        QVERIFY(m_midi.loadData(midiOf({{0, 0, 60}, {24, 0, 62}, {24, 0, 60}, {24, 0, 62}})));
        const auto plan = ClipPlacer::plan(m_map, request());
        QCOMPARE(plan.placements.size(), 4);
        // Each note's own first hit is voice 0, second is voice 1.
        QCOMPARE(plan.placements.at(0).voice, 0); // 60
        QCOMPARE(plan.placements.at(1).voice, 0); // 62
        QCOMPARE(plan.placements.at(2).voice, 1); // 60 again
        QCOMPARE(plan.placements.at(3).voice, 1); // 62 again
    }

    void reportsNotesMissingFromTheMap()
    {
        QVERIFY(m_map.loadJson(mapJson(1)));
        QVERIFY(m_midi.loadData(midiOf({{0, 0, 60}, {24, 0, 99}, {24, 0, 42}, {24, 0, 99}})));
        const auto plan = ClipPlacer::plan(m_map, request());
        QCOMPARE(plan.placements.size(), 1);
        QCOMPARE(plan.unmappedNotes, QList<int>({42, 99})); // distinct and sorted
    }

    void skipsNotesPastTheEnd()
    {
        QVERIFY(m_map.loadJson(mapJson(1)));
        QVERIFY(m_midi.loadData(midiOf({{0, 0, 60}, {96, 0, 60}, {96, 0, 60}})));
        // Only frame 0 fits under 10.
        const auto plan = ClipPlacer::plan(m_map, request(10));
        QCOMPARE(plan.placements.size(), 1);
        QCOMPARE(plan.skippedOutOfRange, 2);
    }

    void trimsAStrikeThatOverhangsTheEnd()
    {
        QVERIFY(m_map.loadJson(mapJson(1)));
        QVERIFY(m_midi.loadData(midiOf({{0, 0, 60}})));
        // A 10-frame clip at frame 0 on a 6-frame timeline must be cut to 6.
        const auto plan = ClipPlacer::plan(m_map, request(6, 10));
        QCOMPARE(plan.placements.size(), 1);
        QCOMPARE(plan.placements.at(0).durationFrames, 6);
    }

    void respectsChannelFilter()
    {
        QVERIFY(m_map.loadJson(mapJson(1)));
        QVERIFY(m_midi.loadData(midiOf({{0, 0, 60}, {24, 5, 60}, {24, 5, 62}})));
        auto r = request();
        r.channel = 5;
        const auto plan = ClipPlacer::plan(m_map, r);
        QCOMPARE(plan.placements.size(), 2);
    }

    void placementsAreInTimeOrder()
    {
        QVERIFY(m_map.loadJson(mapJson(4)));
        QVERIFY(m_midi.loadData(
            midiOf({{0, 0, 60}, {0, 0, 62}, {24, 0, 60}, {24, 0, 62}, {24, 0, 60}})));
        const auto plan = ClipPlacer::plan(m_map, request());
        for (int i = 1; i < plan.placements.size(); i++)
            QVERIFY(plan.placements.at(i).frame >= plan.placements.at(i - 1).frame);
    }

    void rejectsIncompleteRequests()
    {
        QVERIFY(m_map.loadJson(mapJson(1)));
        QVERIFY(m_midi.loadData(midiOf({{0, 0, 60}})));

        QVERIFY(ClipPlacer::plan(NoteMap(), request()).isEmpty());   // no map

        auto noMidi = request();
        noMidi.midi = nullptr;
        QVERIFY(ClipPlacer::plan(m_map, noMidi).isEmpty());

        QVERIFY(ClipPlacer::plan(m_map, request(0, 10)).isEmpty());  // no timeline
        QVERIFY(ClipPlacer::plan(m_map, request(1000, 0)).isEmpty()); // no clip length

        auto badFps = request();
        badFps.fps = 0.0;
        QVERIFY(ClipPlacer::plan(m_map, badFps).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestClipPlacer)

#include "test_clip_placer.moc"
