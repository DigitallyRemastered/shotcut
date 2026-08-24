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

#ifndef BEATGRID_H
#define BEATGRID_H

#include <QVector>

/*!
 * \brief Frame positions of a musical beat grid.
 *
 * Converting beats to frames happens here and nowhere else, so rounding is
 * applied exactly once per position and every consumer agrees on where a beat
 * falls. Pure arithmetic with no Qt object, timeline or undo dependency.
 */
namespace BeatGrid {

struct Params
{
    double bpm{120.0};       //!< Beats per minute. Must be > 0.
    double offsetBeats{0.0}; //!< Grid start, in beats. May be fractional or negative.
    int lengthFrames{0};     //!< Exclusive upper bound. Must be > 0.
    double fps{25.0};        //!< Frame rate. Must be > 0.
    int everyNthBeat{1};     //!< 1 = every beat, 4 = every bar in 4/4. Must be > 0.
};

//! False when the parameters cannot describe a grid at all.
bool isUsable(const Params &params);

//! Frame of the position at \a index, without range checking.
int frameAt(const Params &params, int index);

//! How many positions fall inside [0, lengthFrames). Zero when unusable.
int count(const Params &params);

//! Every in-range frame, ascending.
QVector<int> frames(const Params &params);

} // namespace BeatGrid

#endif // BEATGRID_H
