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

#ifndef MIDISOURCE_H
#define MIDISOURCE_H

#include <QByteArray>
#include <QList>
#include <QString>
#include <QVector>

/*!
 * \brief Reads note onset times out of a Standard MIDI File.
 *
 * Only what is needed to drive an editing time grid: note-on events converted
 * to seconds through the file's tempo map. Control changes, pitch bend and
 * sysex are parsed far enough to be stepped over correctly, then discarded.
 *
 * Parsing never throws and never trusts the file: a truncated or malformed
 * chunk stops that track and leaves whatever was read before it intact.
 */
class MidiSource
{
public:
    struct Note
    {
        int channel{0};    //!< 0-based, as stored in the status byte.
        int note{0};       //!< MIDI note number, 0..127. 60 is middle C.
        int velocity{0};   //!< 1..127. Note-on with velocity 0 is a note-off and is dropped.
        qint64 tick{0};    //!< Absolute tick from the start of the file.
        double seconds{0}; //!< Onset time, resolved through the tempo map.
    };

    bool load(const QString &path, QString *errorMessage = nullptr);
    bool loadData(const QByteArray &data, QString *errorMessage = nullptr);

    bool isValid() const { return m_division != 0 && !m_notes.isEmpty(); }
    void clear();

    //! Channels that carry at least one note, ascending.
    QList<int> channels() const;

    /*!
     * \brief Note onset times in seconds, ascending.
     * \param channel restrict to this channel, or -1 for every channel.
     *
     * Simultaneous notes (a chord, or several tracks striking together)
     * collapse to a single entry, because a time grid wants distinct instants.
     */
    QVector<double> onsets(int channel = -1) const;

    const QVector<Note> &notes() const { return m_notes; }
    double durationSeconds() const;
    double initialTempoBpm() const;

    int division() const { return m_division; }
    int format() const { return m_format; }
    int trackCount() const { return m_trackCount; }

private:
    struct TempoChange
    {
        qint64 tick;
        quint32 usPerQuarter;
    };

    void resolveSeconds();

    QVector<Note> m_notes;
    QVector<TempoChange> m_tempos;
    int m_division{0}; //!< >0 ticks per quarter note; <0 encodes SMPTE.
    int m_format{0};
    int m_trackCount{0};
};

#endif // MIDISOURCE_H
