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

#include "rhythm/midisource.h"
#include "rhythm/rhythmsource.h"

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

//! One track, 96 ticks per quarter, default 120 BPM: a quarter is 0.5 s.
QByteArray midiWithNotes(const QVector<QPair<quint32, int>> &deltaAndChannel)
{
    QByteArray events;
    for (const auto &entry : deltaAndChannel) {
        events += vlq(entry.first);
        events.append(char(0x90 | (entry.second & 0x0f)));
        events.append(char(60));
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

} // namespace

class TestRhythmSource : public QObject
{
    Q_OBJECT

private slots:
    void beatsDelegateToBeatGrid()
    {
        RhythmSource source;
        BeatGrid::Params grid;
        grid.bpm = 120.0;
        grid.fps = 25.0;
        source.setKind(RhythmSource::Beats);
        source.setBeatGrid(grid);

        const auto frames = source.frames(250, 25.0);
        QCOMPARE(frames.size(), 20);
        QCOMPARE(frames.first(), 0);
        QCOMPARE(frames.last(), 238);
    }

    void beatsTakeLengthFromTheCaller()
    {
        RhythmSource source;
        BeatGrid::Params grid;
        grid.bpm = 120.0;
        grid.fps = 25.0;
        grid.lengthFrames = 10; // must be overridden by the frames() argument
        source.setBeatGrid(grid);
        QCOMPARE(source.frames(250, 25.0).size(), 20);
    }

    void midiOnsetsBecomeFrames()
    {
        // Quarter notes at 120 BPM are 0.5 s apart; at 25 fps that is 12 or 13
        // frames after rounding.
        MidiSource midi;
        QVERIFY(midi.loadData(midiWithNotes({{0, 0}, {96, 0}, {96, 0}})));

        RhythmSource source;
        source.setKind(RhythmSource::Midi);
        source.setMidi(&midi);

        const auto frames = source.frames(1000, 25.0);
        QCOMPARE(frames.size(), 3);
        QCOMPARE(frames.at(0), 0);
        QCOMPARE(frames.at(1), 13); // 0.5 s * 25 fps = 12.5, rounds to 13
        QCOMPARE(frames.at(2), 25);
    }

    void midiRespectsChannelFilter()
    {
        MidiSource midi;
        QVERIFY(midi.loadData(midiWithNotes({{0, 0}, {96, 3}, {96, 3}})));

        RhythmSource source;
        source.setKind(RhythmSource::Midi);
        source.setMidi(&midi, 3);
        QCOMPARE(source.frames(1000, 25.0).size(), 2);

        source.setMidi(&midi, 0);
        QCOMPARE(source.frames(1000, 25.0).size(), 1);
    }

    void midiStopsAtTheEndOfTheTimeline()
    {
        MidiSource midi;
        QVERIFY(midi.loadData(midiWithNotes({{0, 0}, {96, 0}, {96, 0}})));

        RhythmSource source;
        source.setKind(RhythmSource::Midi);
        source.setMidi(&midi);
        // Only the first two onsets (frames 0 and 13) fit under 20.
        QCOMPARE(source.frames(20, 25.0).size(), 2);
    }

    void midiDeduplicatesAtLowFrameRates()
    {
        // At 1 fps every onset in this file rounds onto frame 0 or 1.
        MidiSource midi;
        QVERIFY(midi.loadData(midiWithNotes({{0, 0}, {24, 0}, {24, 0}, {24, 0}})));

        RhythmSource source;
        source.setKind(RhythmSource::Midi);
        source.setMidi(&midi);

        const auto frames = source.frames(100, 1.0);
        for (int i = 1; i < frames.size(); i++)
            QVERIFY(frames.at(i) > frames.at(i - 1));
    }

    void midiWithoutASourceYieldsNothing()
    {
        RhythmSource source;
        source.setKind(RhythmSource::Midi);
        QVERIFY(source.frames(1000, 25.0).isEmpty());

        MidiSource empty;
        source.setMidi(&empty);
        QVERIFY(source.frames(1000, 25.0).isEmpty());
    }

    void rejectsNonsenseArguments()
    {
        MidiSource midi;
        QVERIFY(midi.loadData(midiWithNotes({{0, 0}, {96, 0}})));
        RhythmSource source;
        source.setKind(RhythmSource::Midi);
        source.setMidi(&midi);
        QVERIFY(source.frames(0, 25.0).isEmpty());
        QVERIFY(source.frames(1000, 0.0).isEmpty());
        QVERIFY(source.frames(1000, -25.0).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestRhythmSource)

#include "test_rhythm_source.moc"
