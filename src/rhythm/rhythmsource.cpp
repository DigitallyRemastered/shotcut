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

#include "rhythmsource.h"

#include "rhythm/midisource.h"

#include <cmath>

void RhythmSource::setBeatGrid(const BeatGrid::Params &params)
{
    m_grid = params;
}

void RhythmSource::setMidi(const MidiSource *midi, int channel)
{
    m_midi = midi;
    m_midiChannel = channel;
}

QVector<int> RhythmSource::frames(int lengthFrames, double fps) const
{
    if (m_kind == Beats) {
        BeatGrid::Params grid = m_grid;
        grid.lengthFrames = lengthFrames;
        return BeatGrid::frames(grid);
    }

    QVector<int> result;
    if (!m_midi || !m_midi->isValid() || lengthFrames <= 0 || fps <= 0.0)
        return result;

    // Onsets are already ascending and distinct in seconds, but rounding to
    // frames can collapse neighbours, so de-duplicate again here.
    const QVector<double> onsets = m_midi->onsets(m_midiChannel);
    result.reserve(onsets.size());
    int previous = -1;
    for (double seconds : onsets) {
        if (seconds < 0.0)
            continue;
        const int frame = int(std::llround(seconds * fps));
        if (frame >= lengthFrames)
            break; // onsets ascend, so nothing later can fit either
        if (frame == previous)
            continue;
        previous = frame;
        result.append(frame);
    }
    return result;
}
