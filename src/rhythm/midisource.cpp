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

#include "midisource.h"

#include <QFile>
#include <QObject>
#include <QSet>
#include <cstring>
#include <algorithm>

namespace {

constexpr quint32 kDefaultTempo = 500000; //!< 120 BPM, the SMF default.

/*!
 * \brief Bounds-checked forward reader over the file bytes.
 *
 * Every accessor checks before reading and latches \c bad on overrun, so a
 * truncated file yields zeros instead of reading out of bounds.
 */
class Reader
{
public:
    Reader(const uchar *data, qsizetype size)
        : m_data(data)
        , m_size(size)
    {}

    bool atEnd() const { return m_pos >= m_size; }
    bool bad() const { return m_bad; }
    qsizetype pos() const { return m_pos; }
    void seek(qsizetype pos) { m_pos = qBound(qsizetype(0), pos, m_size); }
    bool has(qsizetype count) const { return m_pos + count <= m_size; }

    quint8 u8()
    {
        if (!has(1)) {
            m_bad = true;
            return 0;
        }
        return m_data[m_pos++];
    }

    quint16 u16() { return quint16(quint16(u8()) << 8 | u8()); }

    quint32 u32()
    {
        quint32 value = 0;
        for (int i = 0; i < 4; i++)
            value = value << 8 | u8();
        return value;
    }

    //! Variable-length quantity. The spec caps these at four bytes.
    quint32 vlq()
    {
        quint32 value = 0;
        for (int i = 0; i < 4; i++) {
            const quint8 byte = u8();
            value = (value << 7) | (byte & 0x7f);
            if (!(byte & 0x80))
                return value;
        }
        m_bad = true;
        return value;
    }

    void skip(qsizetype count)
    {
        if (!has(count)) {
            m_bad = true;
            m_pos = m_size;
        } else {
            m_pos += count;
        }
    }

    const uchar *raw() const { return m_data + m_pos; }

private:
    const uchar *m_data;
    qsizetype m_size;
    qsizetype m_pos{0};
    bool m_bad{false};
};

} // namespace

void MidiSource::clear()
{
    m_notes.clear();
    m_tempos.clear();
    m_division = 0;
    m_format = 0;
    m_trackCount = 0;
}

bool MidiSource::load(const QString &path, QString *errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        clear();
        if (errorMessage)
            *errorMessage = QObject::tr("Unable to open %1").arg(path);
        return false;
    }
    return loadData(file.readAll(), errorMessage);
}

bool MidiSource::loadData(const QByteArray &data, QString *errorMessage)
{
    clear();
    const auto fail = [&](const QString &why) {
        clear();
        if (errorMessage)
            *errorMessage = why;
        return false;
    };

    if (data.size() < 14 || !data.startsWith("MThd"))
        return fail(QObject::tr("Not a MIDI file"));

    Reader reader(reinterpret_cast<const uchar *>(data.constData()), data.size());
    reader.skip(4); // "MThd"
    const quint32 headerLength = reader.u32();
    m_format = reader.u16();
    m_trackCount = reader.u16();
    m_division = qint16(reader.u16()); // signed: negative means SMPTE
    if (headerLength > 6)
        reader.skip(headerLength - 6); // forward compatible with a longer header
    if (m_division == 0)
        return fail(QObject::tr("Invalid MIDI time division"));

    int tracksParsed = 0;
    while (reader.has(8) && !reader.bad()) {
        const bool isTrack = std::memcmp(reader.raw(), "MTrk", 4) == 0;
        reader.skip(4);
        const quint32 chunkLength = reader.u32();
        if (!reader.has(chunkLength))
            break; // truncated trailing chunk: keep what we already have
        const qsizetype chunkEnd = reader.pos() + chunkLength;
        if (!isTrack) {
            reader.seek(chunkEnd); // unknown chunk type: the spec says skip it
            continue;
        }
        tracksParsed++;

        qint64 tick = 0;
        quint8 runningStatus = 0;
        while (reader.pos() < chunkEnd && !reader.bad()) {
            tick += reader.vlq();
            if (!reader.has(1))
                break; // delta time ran to the end of the chunk

            quint8 status = reader.raw()[0];
            if (status & 0x80) {
                reader.skip(1);
                runningStatus = status;
            } else {
                status = runningStatus;
                if (!status)
                    break; // data byte with no preceding status: give up on this track
            }

            const quint8 kind = status & 0xf0;
            if (kind == 0x90) { // note on
                const int note = reader.u8();
                const int velocity = reader.u8();
                if (velocity > 0)
                    m_notes.append({status & 0x0f, note, velocity, tick, 0.0});
            } else if (kind == 0x80 || kind == 0xa0 || kind == 0xb0 || kind == 0xe0) {
                reader.skip(2); // note off, aftertouch, control change, pitch bend
            } else if (kind == 0xc0 || kind == 0xd0) {
                reader.skip(1); // program change, channel pressure
            } else if (status == 0xff) {
                const quint8 type = reader.u8();
                const quint32 length = reader.vlq();
                if (type == 0x51 && length == 3) { // set tempo
                    quint32 us = quint32(reader.u8()) << 16;
                    us |= quint32(reader.u8()) << 8;
                    us |= reader.u8();
                    m_tempos.append({tick, us ? us : kDefaultTempo});
                } else if (type == 0x2f) { // end of track
                    reader.skip(length);
                    break;
                } else {
                    reader.skip(length);
                }
            } else if (status == 0xf0 || status == 0xf7) {
                reader.skip(reader.vlq()); // sysex
            } else {
                break; // unrecognised status: abandon this track, keep the rest
            }
        }
        reader.seek(chunkEnd);
    }

    if (tracksParsed == 0)
        return fail(QObject::tr("MIDI file contains no tracks"));

    resolveSeconds();

    // Tracks are stored one after another, so the combined list is only sorted
    // within each track. A time grid needs it globally ordered.
    std::stable_sort(m_notes.begin(), m_notes.end(), [](const Note &a, const Note &b) {
        return a.tick < b.tick;
    });

    if (m_notes.isEmpty())
        return fail(QObject::tr("MIDI file contains no notes"));
    return true;
}

void MidiSource::resolveSeconds()
{
    if (m_division < 0) {
        // SMPTE: the high byte is a negative frame rate, the low byte is ticks
        // per frame. Absolute, so the tempo map does not apply.
        const int fps = -(m_division >> 8);
        const int ticksPerFrame = m_division & 0xff;
        const double secondsPerTick = (fps > 0 && ticksPerFrame > 0)
                                          ? 1.0 / (double(fps) * double(ticksPerFrame))
                                          : 0.0;
        for (auto &note : m_notes)
            note.seconds = double(note.tick) * secondsPerTick;
        return;
    }

    std::stable_sort(m_tempos.begin(), m_tempos.end(), [](const TempoChange &a, const TempoChange &b) {
        return a.tick < b.tick;
    });
    if (m_tempos.isEmpty() || m_tempos.first().tick > 0)
        m_tempos.prepend({0, kDefaultTempo});

    // Elapsed seconds at each tempo change, so resolving a note is a lookup
    // plus one multiply instead of a walk from the beginning.
    QVector<double> elapsed(m_tempos.size(), 0.0);
    for (int i = 1; i < m_tempos.size(); i++) {
        const double quarters = double(m_tempos[i].tick - m_tempos[i - 1].tick) / double(m_division);
        elapsed[i] = elapsed[i - 1] + quarters * (double(m_tempos[i - 1].usPerQuarter) / 1e6);
    }

    for (auto &note : m_notes) {
        // Last tempo change at or before this note.
        int lo = 0;
        int hi = m_tempos.size() - 1;
        while (lo < hi) {
            const int mid = (lo + hi + 1) / 2;
            if (m_tempos[mid].tick <= note.tick)
                lo = mid;
            else
                hi = mid - 1;
        }
        const double quarters = double(note.tick - m_tempos[lo].tick) / double(m_division);
        note.seconds = elapsed[lo] + quarters * (double(m_tempos[lo].usPerQuarter) / 1e6);
    }
}

QList<int> MidiSource::channels() const
{
    QSet<int> seen;
    for (const auto &note : m_notes)
        seen.insert(note.channel);
    QList<int> result(seen.cbegin(), seen.cend());
    std::sort(result.begin(), result.end());
    return result;
}

QVector<double> MidiSource::onsets(int channel) const
{
    QVector<double> result;
    result.reserve(m_notes.size());
    for (const auto &note : m_notes) {
        if (channel >= 0 && note.channel != channel)
            continue;
        // Notes are tick-sorted, so a chord shows up as an immediate repeat.
        if (!result.isEmpty() && qFuzzyCompare(result.last() + 1.0, note.seconds + 1.0))
            continue;
        result.append(note.seconds);
    }
    return result;
}

double MidiSource::durationSeconds() const
{
    return m_notes.isEmpty() ? 0.0 : m_notes.last().seconds;
}

double MidiSource::initialTempoBpm() const
{
    const quint32 us = m_tempos.isEmpty() ? kDefaultTempo : m_tempos.first().usPerQuarter;
    return us ? 60000000.0 / double(us) : 0.0;
}
