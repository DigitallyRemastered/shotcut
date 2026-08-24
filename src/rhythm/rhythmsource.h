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

#ifndef RHYTHMSOURCE_H
#define RHYTHMSOURCE_H

#include "rhythm/beatgrid.h"

#include <QVector>

class MidiSource;

/*!
 * \brief Where the instants come from that a generator acts on.
 *
 * Wraps the two ways of saying "do something here, here and here": an even
 * tempo grid, or the note onsets of a MIDI performance. Consumers ask for
 * frames and do not care which was chosen.
 */
class RhythmSource
{
public:
    enum Kind {
        Beats, //!< Evenly spaced from a tempo.
        Midi,  //!< Note onsets from a MIDI file.
    };

    Kind kind() const { return m_kind; }
    void setKind(Kind kind) { m_kind = kind; }

    void setBeatGrid(const BeatGrid::Params &params);
    BeatGrid::Params beatGrid() const { return m_grid; }

    /*!
     * \brief Use \a midi as the onset source.
     * \param channel MIDI channel to take notes from, or -1 for all.
     *
     * The source is not owned and must outlive this object.
     */
    void setMidi(const MidiSource *midi, int channel = -1);

    /*!
     * \brief Instants as frame numbers, ascending and distinct.
     * \param lengthFrames exclusive upper bound.
     * \param fps used to convert MIDI seconds to frames; ignored for Beats,
     *        which reads the frame rate from its own parameters.
     */
    QVector<int> frames(int lengthFrames, double fps) const;

private:
    Kind m_kind{Beats};
    BeatGrid::Params m_grid;
    const MidiSource *m_midi{nullptr};
    int m_midiChannel{-1};
};

#endif // RHYTHMSOURCE_H
