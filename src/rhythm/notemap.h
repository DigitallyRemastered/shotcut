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

#ifndef NOTEMAP_H
#define NOTEMAP_H

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QRectF>
#include <QString>

/*!
 * \brief Where on screen each MIDI note should be struck.
 *
 * Describes a physical instrument as data: which note lands where, which clip
 * plays when it is struck, and how many parallel tracks to spread repeats
 * across. Written as JSON so a new instrument needs no code, only a file.
 *
 * \code
 * {
 *   "name": "Xylophone",
 *   "sourceClip": "clips/fastclick2.webm",
 *   "voices": 4,
 *   "notes": [
 *     { "note": 55, "track": "G4", "rect": [454.067, 695.527, 108, 108], "opacity": 1.0 }
 *   ]
 * }
 * \endcode
 */
class NoteMap
{
public:
    struct Entry
    {
        int note{-1};        //!< MIDI note number, 0..127.
        QString track;       //!< Base name for this note's tracks.
        QRectF rect;         //!< Where the struck clip sits, in profile pixels.
        double opacity{1.0};
    };

    bool load(const QString &path, QString *errorMessage = nullptr);
    bool loadJson(const QByteArray &json, QString *errorMessage = nullptr);
    void clear();

    bool isValid() const { return !m_entries.isEmpty() && !m_sourceClip.isEmpty(); }

    QString name() const { return m_name; }
    QString sourceClip() const { return m_sourceClip; }

    /*!
     * \brief How many parallel tracks each note gets.
     *
     * A note struck again before the previous hit has finished needs somewhere
     * else to go, so hits round-robin across this many tracks. Always >= 1.
     */
    int voices() const { return m_voices; }

    //! Null when the note is not in the map.
    const Entry *entry(int note) const;

    QList<int> notes() const;
    int count() const { return int(m_entries.size()); }

    //! Track name for one voice of a note, e.g. "G4", "G4b", "G4c".
    static QString trackName(const QString &base, int voice);

private:
    QString m_name;
    QString m_sourceClip;
    int m_voices{1};
    QHash<int, Entry> m_entries;
};

#endif // NOTEMAP_H
