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

//! Encode a variable-length quantity the way the SMF spec defines it.
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

QByteArray header(quint16 format, quint16 tracks, qint16 division)
{
    QByteArray out("MThd");
    appendU32(out, 6);
    appendU16(out, format);
    appendU16(out, tracks);
    appendU16(out, quint16(division));
    return out;
}

QByteArray track(const QByteArray &events)
{
    QByteArray body = events;
    body += vlq(0);
    body.append(char(0xff)); // end of track
    body.append(char(0x2f));
    body.append(char(0x00));
    QByteArray out("MTrk");
    appendU32(out, quint32(body.size()));
    return out + body;
}

QByteArray noteOn(quint32 delta, int channel, int note, int velocity)
{
    QByteArray out = vlq(delta);
    out.append(char(0x90 | (channel & 0x0f)));
    out.append(char(note));
    out.append(char(velocity));
    return out;
}

//! A note-on with no status byte, relying on the previous one.
QByteArray runningNote(quint32 delta, int note, int velocity)
{
    QByteArray out = vlq(delta);
    out.append(char(note));
    out.append(char(velocity));
    return out;
}

QByteArray setTempo(quint32 delta, quint32 usPerQuarter)
{
    QByteArray out = vlq(delta);
    out.append(char(0xff));
    out.append(char(0x51));
    out.append(char(0x03));
    out.append(char((usPerQuarter >> 16) & 0xff));
    out.append(char((usPerQuarter >> 8) & 0xff));
    out.append(char(usPerQuarter & 0xff));
    return out;
}

QByteArray controlChange(quint32 delta, int channel, int cc, int value)
{
    QByteArray out = vlq(delta);
    out.append(char(0xb0 | (channel & 0x0f)));
    out.append(char(cc));
    out.append(char(value));
    return out;
}

} // namespace

class TestMidiSource : public QObject
{
    Q_OBJECT

private slots:
    void rejectsNonMidiData()
    {
        MidiSource midi;
        QString error;
        QVERIFY(!midi.loadData(QByteArray("this is not a midi file at all"), &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!midi.isValid());
    }

    void rejectsEmptyData()
    {
        MidiSource midi;
        QVERIFY(!midi.loadData(QByteArray()));
        QVERIFY(!midi.isValid());
    }

    void parsesSimpleTrack()
    {
        // 96 ticks per quarter, default 120 BPM => a quarter note is 0.5 s.
        QByteArray data = header(0, 1, 96);
        data += track(noteOn(0, 0, 60, 100) + noteOn(96, 0, 62, 100) + noteOn(96, 0, 64, 100));

        MidiSource midi;
        QVERIFY(midi.loadData(data));
        QCOMPARE(midi.division(), 96);
        QCOMPARE(midi.notes().size(), 3);
        QCOMPARE(midi.notes().at(0).note, 60);
        QCOMPARE(midi.initialTempoBpm(), 120.0);

        const auto onsets = midi.onsets();
        QCOMPARE(onsets.size(), 3);
        QVERIFY(qAbs(onsets.at(0) - 0.0) < 1e-9);
        QVERIFY(qAbs(onsets.at(1) - 0.5) < 1e-9);
        QVERIFY(qAbs(onsets.at(2) - 1.0) < 1e-9);
    }

    void dropsNoteOnWithZeroVelocity()
    {
        QByteArray data = header(0, 1, 96);
        // The second event is the conventional "note off" spelling.
        data += track(noteOn(0, 0, 60, 100) + noteOn(96, 0, 60, 0));

        MidiSource midi;
        QVERIFY(midi.loadData(data));
        QCOMPARE(midi.notes().size(), 1);
        QCOMPARE(midi.notes().at(0).velocity, 100);
    }

    void honoursRunningStatus()
    {
        QByteArray data = header(0, 1, 96);
        data += track(noteOn(0, 0, 60, 100) + runningNote(96, 62, 100) + runningNote(96, 64, 100));

        MidiSource midi;
        QVERIFY(midi.loadData(data));
        QCOMPARE(midi.notes().size(), 3);
        QCOMPARE(midi.notes().at(2).note, 64);
        QVERIFY(qAbs(midi.notes().at(2).seconds - 1.0) < 1e-9);
    }

    void stepsOverControlChanges()
    {
        QByteArray data = header(0, 1, 96);
        data += track(controlChange(0, 0, 7, 100) + noteOn(96, 0, 60, 100));

        MidiSource midi;
        QVERIFY(midi.loadData(data));
        QCOMPARE(midi.notes().size(), 1);
        QVERIFY(qAbs(midi.notes().at(0).seconds - 0.5) < 1e-9);
    }

    void appliesTempoChanges()
    {
        // Start at 120 BPM (0.5 s per quarter), double to 240 BPM after one
        // quarter, so the third note is 0.25 s after the second, not 0.5 s.
        QByteArray data = header(0, 1, 96);
        data += track(setTempo(0, 500000) + noteOn(0, 0, 60, 100) + noteOn(96, 0, 62, 100)
                      + setTempo(0, 250000) + noteOn(96, 0, 64, 100));

        MidiSource midi;
        QVERIFY(midi.loadData(data));
        QCOMPARE(midi.notes().size(), 3);
        const auto onsets = midi.onsets();
        QVERIFY(qAbs(onsets.at(0) - 0.00) < 1e-9);
        QVERIFY(qAbs(onsets.at(1) - 0.50) < 1e-9);
        QVERIFY(qAbs(onsets.at(2) - 0.75) < 1e-9);
    }

    void supportsSmpteDivision()
    {
        // -25 fps, 40 ticks per frame => 1000 ticks per second, tempo ignored.
        const qint16 division = qint16((quint16(256 - 25) << 8) | 40);
        QByteArray data = header(0, 1, division);
        data += track(noteOn(0, 0, 60, 100) + noteOn(1000, 0, 62, 100));

        MidiSource midi;
        QVERIFY(midi.loadData(data));
        QCOMPARE(midi.notes().size(), 2);
        QVERIFY(qAbs(midi.notes().at(1).seconds - 1.0) < 1e-9);
    }

    void sortsNotesAcrossTracks()
    {
        // Two tracks stored one after the other, interleaved in time. The
        // combined list must come back in time order, not file order.
        QByteArray data = header(1, 2, 96);
        data += track(noteOn(0, 0, 60, 100) + noteOn(192, 0, 64, 100));
        data += track(noteOn(96, 1, 62, 100));

        MidiSource midi;
        QVERIFY(midi.loadData(data));
        QCOMPARE(midi.notes().size(), 3);
        QCOMPARE(midi.notes().at(0).note, 60);
        QCOMPARE(midi.notes().at(1).note, 62);
        QCOMPARE(midi.notes().at(2).note, 64);
    }

    void collapsesSimultaneousNotes()
    {
        // A three-note chord is one instant on a time grid.
        QByteArray data = header(0, 1, 96);
        data += track(noteOn(0, 0, 60, 100) + noteOn(0, 0, 64, 100) + noteOn(0, 0, 67, 100)
                      + noteOn(96, 0, 72, 100));

        MidiSource midi;
        QVERIFY(midi.loadData(data));
        QCOMPARE(midi.notes().size(), 4);
        QCOMPARE(midi.onsets().size(), 2);
    }

    void filtersByChannel()
    {
        QByteArray data = header(0, 1, 96);
        data += track(noteOn(0, 0, 60, 100) + noteOn(96, 3, 62, 100) + noteOn(96, 3, 64, 100));

        MidiSource midi;
        QVERIFY(midi.loadData(data));
        QCOMPARE(midi.channels(), QList<int>({0, 3}));
        QCOMPARE(midi.onsets(0).size(), 1);
        QCOMPARE(midi.onsets(3).size(), 2);
        QCOMPARE(midi.onsets(9).size(), 0);
        QCOMPARE(midi.onsets(-1).size(), 3);
    }

    void skipsUnknownChunks()
    {
        QByteArray data = header(0, 1, 96);
        QByteArray junk("XYZW");
        appendU32(junk, 4);
        junk.append("\x01\x02\x03\x04", 4);
        data += junk;
        data += track(noteOn(0, 0, 60, 100));

        MidiSource midi;
        QVERIFY(midi.loadData(data));
        QCOMPARE(midi.notes().size(), 1);
    }

    void survivesTruncation()
    {
        QByteArray full = header(0, 1, 96);
        full += track(noteOn(0, 0, 60, 100) + noteOn(96, 0, 62, 100) + noteOn(96, 0, 64, 100));

        // Chop at every length; none may crash or hang.
        for (int size = 0; size < full.size(); size++) {
            MidiSource midi;
            midi.loadData(full.left(size));
            for (const auto &note : midi.notes()) {
                QVERIFY(note.tick >= 0);
                QVERIFY(note.seconds >= 0.0);
            }
        }
        QVERIFY(true);
    }

    void clearResetsState()
    {
        QByteArray data = header(0, 1, 96);
        data += track(noteOn(0, 0, 60, 100));
        MidiSource midi;
        QVERIFY(midi.loadData(data));
        QVERIFY(midi.isValid());
        midi.clear();
        QVERIFY(!midi.isValid());
        QVERIFY(midi.notes().isEmpty());
        QCOMPARE(midi.division(), 0);
    }
};

QTEST_GUILESS_MAIN(TestMidiSource)

#include "test_midi_source.moc"
