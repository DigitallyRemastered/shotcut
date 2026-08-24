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

#ifndef PATTERN_H
#define PATTERN_H

#include "models/keyframesmodel.h"

#include <QString>
#include <QVector>

/*!
 * \brief One repetition of a parameter move, stamped at every rhythmic instant.
 *
 * A pulse is described relative to the instant it fires on: \c pulseFrames
 * holds frame offsets and \c values the parameter value at each. A pulse of
 * offsets {0, 4, 6} with values {1, 2, 1} is "jump to 2 over four frames, then
 * settle back over two".
 */
struct Pattern
{
    QVector<int> pulseFrames;    //!< Offsets from the instant. Ascending, first is usually 0.
    QVector<double> values;      //!< One per entry in pulseFrames.
    KeyframesModel::InterpolationType interpolation{KeyframesModel::SmoothNaturalInterpolation};
    QString name;                //!< For the preset list. Not used in generation.

    //! Same number of offsets as values, at least one of each, offsets ascending.
    bool isValid() const;

    //! Frames from the first offset to the last. Zero for a single-offset pulse.
    int span() const;
};

/*!
 * \brief Turns a pattern plus a list of instants into concrete keyframes.
 */
namespace PatternExpander {

struct Keyframe
{
    int frame;
    double value;
};

/*!
 * \brief Stamp \a pattern at every instant in \a times.
 *
 * Results are ascending by frame and carry no duplicates. Where a dense grid
 * makes one pulse overlap the next, the later instant wins, so the rhythm stays
 * audible rather than being masked by the tail of the previous pulse.
 *
 * Frames outside [0, lengthFrames) are dropped rather than clamped, since
 * clamping would pile several keyframes onto frame zero.
 */
QVector<Keyframe> expand(const Pattern &pattern, const QVector<int> &times, int lengthFrames);

//! True when the pulse is longer than the gap between instants.
bool overlaps(const Pattern &pattern, const QVector<int> &times);

} // namespace PatternExpander

#endif // PATTERN_H
